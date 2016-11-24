//
// i2cmaster.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
// 
// Large portions are:
//	Copyright (C) 2011-2013 Mike McCauley
//	Copyright (C) 2014, 2015, 2016 by Arjan van Vught <info@raspberrypi-dmx.nl>
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
#include <circle/i2cmaster.h>
#include <circle/memio.h>
#include <circle/bcm2835.h>
#include <circle/machineinfo.h>
#include <circle/synchronize.h>
#include <assert.h>

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

CI2CMaster::CI2CMaster (unsigned nDevice, boolean bFastMode)
:	m_nDevice (nDevice),
	m_nBaseAddress (nDevice == 0 ? ARM_BSC0_BASE : ARM_BSC1_BASE),
	m_bFastMode (bFastMode),
	m_SDA (nDevice == 0 ? 0 : 2, GPIOModeAlternateFunction0),
	m_SCL (nDevice == 0 ? 1 : 3, GPIOModeAlternateFunction0),
	m_nCoreClockRate (CMachineInfo::Get ()->GetClockRate (CLOCK_ID_CORE)),
	m_SpinLock (FALSE)
{
	assert (nDevice <= 1);
	assert (m_nCoreClockRate > 0);
}

CI2CMaster::~CI2CMaster (void)
{
	m_nBaseAddress = 0;
}

boolean CI2CMaster::Initialize (void)
{
	SetClock (m_bFastMode ? 400000 : 100000);

	return TRUE;
}

void CI2CMaster::SetClock (unsigned nClockSpeed)
{
	PeripheralEntry ();

	assert (nClockSpeed > 0);
	u16 nDivider = (u16) (m_nCoreClockRate / nClockSpeed);
	write32 (m_nBaseAddress + ARM_BSC_DIV__OFFSET, nDivider);
	
	PeripheralExit ();
}

int CI2CMaster::Read (u8 ucAddress, void *pBuffer, unsigned nCount)
{
	if (ucAddress >= 0x80)
	{
		return -I2C_MASTER_INALID_PARM;
	}

	if (nCount == 0)
	{
		return -I2C_MASTER_INALID_PARM;
	}

	m_SpinLock.Acquire ();

	u8 *pData = (u8 *) pBuffer;
	assert (pData != 0);

	int nResult = 0;

	PeripheralEntry ();

	// setup transfer
	write32 (m_nBaseAddress + ARM_BSC_A__OFFSET, ucAddress);

	write32 (m_nBaseAddress + ARM_BSC_C__OFFSET, C_CLEAR);
	write32 (m_nBaseAddress + ARM_BSC_S__OFFSET, S_CLKT | S_ERR | S_DONE);

	write32 (m_nBaseAddress + ARM_BSC_DLEN__OFFSET, nCount);

	write32 (m_nBaseAddress + ARM_BSC_C__OFFSET, C_I2CEN | C_ST | C_READ);

	// transfer active
	while (!(read32 (m_nBaseAddress + ARM_BSC_S__OFFSET) & S_DONE))
	{
		while (read32 (m_nBaseAddress + ARM_BSC_S__OFFSET) & S_RXD)
		{
			*pData++ = read32 (m_nBaseAddress + ARM_BSC_FIFO__OFFSET) & FIFO__MASK;

			nCount--;
			nResult++;
		}
	}

	// transfer has finished, grab any remaining stuff from FIFO
	while (   nCount > 0
	       && (read32 (m_nBaseAddress + ARM_BSC_S__OFFSET) & S_RXD))
	{
		*pData++ = read32 (m_nBaseAddress + ARM_BSC_FIFO__OFFSET) & FIFO__MASK;

		nCount--;
		nResult++;
	}

	u32 nStatus = read32 (m_nBaseAddress + ARM_BSC_S__OFFSET);
	if (nStatus & S_ERR)
	{
		write32 (m_nBaseAddress + ARM_BSC_S__OFFSET, S_ERR);

		nResult = -I2C_MASTER_ERROR_NACK;
	}
	else if (nStatus & S_CLKT)
	{
		nResult = -I2C_MASTER_ERROR_CLKT;
	}
	else if (nCount > 0)
	{
		nResult = -I2C_MASTER_DATA_LEFT;
	}

	write32 (m_nBaseAddress + ARM_BSC_S__OFFSET, S_DONE);

	PeripheralExit ();

	m_SpinLock.Release ();

	return nResult;
}

int CI2CMaster::Write (u8 ucAddress, const void *pBuffer, unsigned nCount)
{
	if (ucAddress >= 0x80)
	{
		return -I2C_MASTER_INALID_PARM;
	}

	if (nCount != 0 && pBuffer == 0)
	{
		return -I2C_MASTER_INALID_PARM;
	}

	m_SpinLock.Acquire ();

	u8 *pData = (u8 *) pBuffer;

	int nResult = 0;

	PeripheralEntry ();

	// setup transfer
	write32 (m_nBaseAddress + ARM_BSC_A__OFFSET, ucAddress);

	write32 (m_nBaseAddress + ARM_BSC_C__OFFSET, C_CLEAR);
	write32 (m_nBaseAddress + ARM_BSC_S__OFFSET, S_CLKT | S_ERR | S_DONE);

	write32 (m_nBaseAddress + ARM_BSC_DLEN__OFFSET, nCount);

	// fill FIFO
	for (unsigned i = 0; nCount > 0 && i < FIFO_SIZE; i++)
	{
		write32 (m_nBaseAddress + ARM_BSC_FIFO__OFFSET, *pData++);

		nCount--;
		nResult++;
	}

	// start transfer
	write32 (m_nBaseAddress + ARM_BSC_C__OFFSET, C_I2CEN | C_ST);

	// transfer active
	while (!(read32 (m_nBaseAddress + ARM_BSC_S__OFFSET) & S_DONE))
	{
		while (   nCount > 0
		       && (read32 (m_nBaseAddress + ARM_BSC_S__OFFSET) & S_TXD))
		{
			write32 (m_nBaseAddress + ARM_BSC_FIFO__OFFSET, *pData++);

			nCount--;
			nResult++;
		}
	}

	// check status
	u32 nStatus = read32 (m_nBaseAddress + ARM_BSC_S__OFFSET);
	if (nStatus & S_ERR)
	{
		write32 (m_nBaseAddress + ARM_BSC_S__OFFSET, S_ERR);

		nResult = -I2C_MASTER_ERROR_NACK;
	}
	else if (nStatus & S_CLKT)
	{
		nResult = -I2C_MASTER_ERROR_CLKT;
	}
	else if (nCount > 0)
	{
		nResult = -I2C_MASTER_DATA_LEFT;
	}

	write32 (m_nBaseAddress + ARM_BSC_S__OFFSET, S_DONE);

	PeripheralExit ();

	m_SpinLock.Release ();

	return nResult;
}
