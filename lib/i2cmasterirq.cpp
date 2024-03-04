//
// i2cmasterirq.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2024  R. Stange <rsta2@o2online.de>
// 
// Large portions are:
//	Copyright (C) 2011-2013 Mike McCauley
//	Copyright (C) 2014, 2015, 2016 by Arjan van Vught <info@raspberrypi-dmx.nl>
//	Copyright (C) 2024 Sebastien Nicolas <seba1978@gmx.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <circle/i2cmasterirq.h>
#include <circle/memio.h>
#include <circle/bcm2835.h>
#include <circle/machineinfo.h>
#include <circle/synchronize.h>
#include <assert.h>

#if RASPPI < 4
	#define DEVICES		2
#else
	#define DEVICES		7
#endif

#define CONFIGS			3

#define GPIOS			2
#define GPIO_SDA		0
#define GPIO_SCL		1

// Control register
#define C_I2CEN			(1 << 15)
#define C_INTR			(1 << 10)
#define C_INTT			(1 << 9)
#define C_INTD			(1 << 8)
#define C_ST			(1 << 7)
#define C_CLEAR			(1 << 5)
#define C_READ			(1 << 0)

// Status register
#define S_CLKT			(1 << 9)
#define S_ERR			(1 << 8)
#define S_RXF			(1 << 7)
#define S_TXE			(1 << 6)
#define S_RXD			(1 << 5)
#define S_TXD			(1 << 4)
#define S_RXR			(1 << 3)
#define S_TXW			(1 << 2)
#define S_DONE			(1 << 1)
#define S_TA			(1 << 0)

// FIFO register
#define FIFO__MASK		0xFF

#define FIFO_SIZE		16

static uintptr s_BaseAddress[DEVICES] =
{
	ARM_IO_BASE + 0x205000,
	ARM_IO_BASE + 0x804000,
#if RASPPI >= 4
	0,
	ARM_IO_BASE + 0x205600,
	ARM_IO_BASE + 0x205800,
	ARM_IO_BASE + 0x205A80,
	ARM_IO_BASE + 0x205C00
#endif
};

#define NONE	{10000, 10000}

static unsigned s_GPIOConfig[DEVICES][CONFIGS][GPIOS] =
{
	// SDA, SCL
	{{ 0,  1}, {28, 29}, {44, 45}}, // ALT0, ALT0, ALT1
	{{ 2,  3},   NONE,     NONE  }, // ALT0
#if RASPPI >= 4
	{  NONE,     NONE,     NONE  }, // unused
	{{ 2,  3}, { 4,  5},   NONE  }, // ALT5
	{{ 6,  7}, { 8,  9},   NONE  }, // ALT5
	{{10, 11}, {12, 13},   NONE  }, // ALT5
	{{22, 23},   NONE  ,   NONE  }  // ALT5
#endif
};

#define ALT_FUNC(device, config)	(  (device) == 0 && (config) == 2	\
					 ? GPIOModeAlternateFunction1		\
					 : (  (device) < 2			\
					    ? GPIOModeAlternateFunction0	\
					    : GPIOModeAlternateFunction5))

CI2CMasterIRQ::CI2CMasterIRQ (CInterruptSystem *pInterruptSystem, unsigned nDevice, boolean bFastMode, unsigned nConfig)
:	m_nDevice (nDevice),
	m_nBaseAddress (0),
	m_bFastMode (bFastMode),
	m_nConfig (nConfig),
	m_bValid (FALSE),
	m_nCoreClockRate (CMachineInfo::Get ()->GetClockRate (CLOCK_ID_CORE)),
	m_nClockSpeed (0),
	m_SpinLock (TASK_LEVEL),
	m_nStatus (StatusSuccess),
	m_pReadBuffer (0),
	m_nReadCount (0),
	m_pInterruptSystem (pInterruptSystem),
	m_pCompletionRoutine (0),
	m_pCompletionParam (0)
{
	if (   m_nDevice >= DEVICES
	    || m_nConfig >= CONFIGS
	    || s_GPIOConfig[nDevice][nConfig][0] >= GPIO_PINS)
	{
		return;
	}

	m_nBaseAddress = s_BaseAddress[nDevice];
	assert (m_nBaseAddress != 0);

	m_SDA.AssignPin (s_GPIOConfig[nDevice][nConfig][GPIO_SDA]);
	m_SDA.SetMode (ALT_FUNC (nDevice, nConfig));
	m_SDA.SetPullMode (GPIOPullModeUp);

	m_SCL.AssignPin (s_GPIOConfig[nDevice][nConfig][GPIO_SCL]);
	m_SCL.SetMode (ALT_FUNC (nDevice, nConfig));
	m_SCL.SetPullMode (GPIOPullModeUp);

	assert (m_nCoreClockRate > 0);

	m_bValid = TRUE;
}

CI2CMasterIRQ::~CI2CMasterIRQ (void)
{
	if (m_bValid)
	{
		m_SDA.SetMode (GPIOModeInput);
		m_SCL.SetMode (GPIOModeInput);

		m_bValid = FALSE;
	}

	m_nBaseAddress = 0;
}

boolean CI2CMasterIRQ::Initialize (void)
{
	if (!m_bValid)
	{
		return FALSE;
	}

	SetClock (m_bFastMode ? 400000 : 100000);

	PeripheralEntry ();

	assert (m_pInterruptSystem != 0);
	m_pInterruptSystem->ConnectIRQ (ARM_IRQ_I2C, InterruptStub, this);

	PeripheralExit ();

	return TRUE;
}

void CI2CMasterIRQ::SetClock (unsigned nClockSpeed)
{
	assert (m_bValid);

	PeripheralEntry ();

	assert (nClockSpeed > 0);
	m_nClockSpeed = nClockSpeed;

	u16 nDivider = (u16) (m_nCoreClockRate / nClockSpeed);
	write32 (m_nBaseAddress + ARM_BSC_DIV__OFFSET, nDivider);
	
	PeripheralExit ();
}

void CI2CMasterIRQ::SetCompletionRoutine (TI2CCompletionRoutine *pRoutine, void *pParam)
{
	assert (m_pCompletionRoutine == 0);
	m_pCompletionRoutine = pRoutine;
	assert (m_pCompletionRoutine != 0);

	m_pCompletionParam = pParam;
}

void CI2CMasterIRQ::CallCompletionRoutine()
{
	if (m_pCompletionRoutine != 0)
	{
		(*m_pCompletionRoutine) (m_nStatus, m_pCompletionParam);
	}
}

int CI2CMasterIRQ::GetStatus()
{
	return m_nStatus;
}

int CI2CMasterIRQ::Read (u8 ucAddress, void *pBuffer, unsigned nCount)
{
	return StartTransfer(ucAddress, 0, 0, pBuffer, nCount, TRUE, FALSE);
}

int CI2CMasterIRQ::Write (u8 ucAddress, const void *pBuffer, unsigned nCount)
{
	return StartTransfer(ucAddress, pBuffer, nCount, 0, 0, FALSE, TRUE);
}

int CI2CMasterIRQ::StartWriteRead (u8 ucAddress,
				   const void *pWriteBuffer, unsigned nWriteCount,
				   void *pReadBuffer, unsigned nReadCount)
{
	return StartTransfer(ucAddress, pWriteBuffer, nWriteCount, pReadBuffer, nReadCount, TRUE, TRUE);
}

int CI2CMasterIRQ::StartTransfer (u8 ucAddress,
				  const void *pWriteBuffer, unsigned nWriteCount,
				  void *pReadBuffer, unsigned nReadCount,
				  bool bValidateReadBuffer, bool bValidateWriteBuffer)
{
	assert (m_bValid);

	/*if (m_nStatus > 0) {
		return StatusInvalidState;
	}*/

	if (ucAddress >= 0x80)
	{
		return StatusInvalidParam;
	}

	if (nWriteCount > FIFO_SIZE || nReadCount > FIFO_SIZE)
	{
		return StatusInvalidParam;
	}

	if (bValidateReadBuffer && (nReadCount == 0 || pReadBuffer == 0))
	{
		return StatusInvalidParam;
	}

	if (bValidateWriteBuffer && (nWriteCount == 0 || pWriteBuffer == 0))
	{
		return StatusInvalidParam;
	}

	m_SpinLock.Acquire ();
	m_pReadBuffer = pReadBuffer;
	m_nReadCount = nReadCount;

	PeripheralEntry ();

	// setup transfer
	write32 (m_nBaseAddress + ARM_BSC_A__OFFSET, ucAddress);
	write32 (m_nBaseAddress + ARM_BSC_C__OFFSET, C_CLEAR);
	write32 (m_nBaseAddress + ARM_BSC_S__OFFSET, S_CLKT | S_ERR | S_DONE);

	// write request, possibly followed by read request
	if (nWriteCount != 0)
	{
		m_nStatus = StatusWriting;
		write32 (m_nBaseAddress + ARM_BSC_DLEN__OFFSET, nWriteCount);

		// fill FIFO
		u8 *pWriteData = (u8 *) pWriteBuffer;
		for (unsigned i = 0; i < nWriteCount; i++)
		{
			write32 (m_nBaseAddress + ARM_BSC_FIFO__OFFSET, *pWriteData++);
		}

		// start write transfer with DONE interrupt
		write32 (m_nBaseAddress + ARM_BSC_C__OFFSET, C_I2CEN | C_ST | C_INTD);
	}

	// read request without preliminary write
	else
	{
		m_nStatus = StatusReading;

		// start read transfer with DONE interrupt
		write32 (m_nBaseAddress + ARM_BSC_DLEN__OFFSET, m_nReadCount);
		write32 (m_nBaseAddress + ARM_BSC_C__OFFSET, C_I2CEN | C_ST | C_READ | C_INTD);
	}

	PeripheralExit ();

	m_SpinLock.Release ();

	return 0;
}

void CI2CMasterIRQ::InterruptHandler (void)
{
	m_SpinLock.Acquire ();

	PeripheralEntry ();

	u32 nStatus = read32 (m_nBaseAddress + ARM_BSC_S__OFFSET);
	write32 (m_nBaseAddress + ARM_BSC_S__OFFSET, S_CLKT | S_ERR | S_DONE);

	if (nStatus & S_ERR)
	{
		m_nStatus = StatusAckError;
		CallCompletionRoutine();
	}
	else if (nStatus & S_CLKT)
	{
		write32 (m_nBaseAddress + ARM_BSC_S__OFFSET, S_CLKT);
		m_nStatus = StatusClockStretchTimeout;
		CallCompletionRoutine();
	}
	else if (nStatus & S_DONE)
	{
		if (m_nStatus == StatusWriting)
		{
			if (m_nReadCount == 0)
			{
				m_nStatus = StatusSuccess;
				CallCompletionRoutine();
			}
			else
			{
				m_nStatus = StatusReading;

				// start read transfer with DONE interrupt
				write32 (m_nBaseAddress + ARM_BSC_C__OFFSET, C_CLEAR);
				write32 (m_nBaseAddress + ARM_BSC_DLEN__OFFSET, m_nReadCount);
				write32 (m_nBaseAddress + ARM_BSC_C__OFFSET, C_I2CEN | C_ST | C_READ | C_INTD);
			}
		}
		else if (m_nStatus == StatusReading)
		{
			u8 *pReadBuffer = (u8 *) m_pReadBuffer;
			unsigned nReadCount = m_nReadCount;
			while (nReadCount > 0
			       && (read32 (m_nBaseAddress + ARM_BSC_S__OFFSET) & S_RXD))
			{
				*pReadBuffer++ = read32 (m_nBaseAddress + ARM_BSC_FIFO__OFFSET) & FIFO__MASK;
				nReadCount--;
			}

			m_nStatus = nReadCount > 0 ? StatusDataLeftToReadError : StatusSuccess;
			CallCompletionRoutine();
		}
		else
		{
			assert(0);
		}
	}
	else
	{
		assert(0);
	}

	PeripheralExit ();

	m_SpinLock.Release ();
}

void CI2CMasterIRQ::InterruptStub (void *pParam)
{
	CI2CMasterIRQ *pThis = static_cast<CI2CMasterIRQ *> (pParam);
	assert (pThis != 0);

	pThis->InterruptHandler ();
}
