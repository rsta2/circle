//
// spimaster.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2022  R. Stange <rsta2@o2online.de>
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
#include <circle/spimaster.h>
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/machineinfo.h>
#include <circle/synchronize.h>
#include <circle/timer.h>
#include <assert.h>

#if RASPPI < 4
	#define DEVICES		1
#else
	#define DEVICES		7
#endif

#define GPIOS		5
#define GPIO_MISO	0
#define GPIO_MOSI	1
#define GPIO_SCLK	2
#define GPIO_CE0	3
#define GPIO_CE1	4

#define VALUES		2
#define VALUE_PIN	0
#define VALUE_ALT	1

// Registers
#define ARM_SPI_CS	(m_nBaseAddress + 0x00)
#define ARM_SPI_FIFO	(m_nBaseAddress + 0x04)
#define ARM_SPI_CLK	(m_nBaseAddress + 0x08)
#define ARM_SPI_DLEN	(m_nBaseAddress + 0x0C)
#define ARM_SPI_LTOH	(m_nBaseAddress + 0x10)
#define ARM_SPI_DC	(m_nBaseAddress + 0x14)

// CS Register
#define CS_LEN_LONG	(1 << 25)
#define CS_DMA_LEN	(1 << 24)
#define CS_CSPOL2	(1 << 23)
#define CS_CSPOL1	(1 << 22)
#define CS_CSPOL0	(1 << 21)
#define CS_RXF		(1 << 20)
#define CS_RXR		(1 << 19)
#define CS_TXD		(1 << 18)
#define CS_RXD		(1 << 17)
#define CS_DONE		(1 << 16)
#define CS_LEN		(1 << 13)
#define CS_REN		(1 << 12)
#define CS_ADCS		(1 << 11)
#define CS_INTR		(1 << 10)
#define CS_INTD		(1 << 9)
#define CS_DMAEN	(1 << 8)
#define CS_TA		(1 << 7)
#define CS_CSPOL	(1 << 6)
#define CS_CLEAR_RX	(1 << 5)
#define CS_CLEAR_TX	(1 << 4)
#define CS_CPOL__SHIFT	3
#define CS_CPHA__SHIFT	2
#define CS_CS		(3 << 0)
#define CS_CS__SHIFT	0

static uintptr s_BaseAddress[DEVICES] =
{
	ARM_IO_BASE + 0x204000,
#if RASPPI >= 4
	0,
	0,
	ARM_IO_BASE + 0x204600,
	ARM_IO_BASE + 0x204800,
	ARM_IO_BASE + 0x204A00,
	ARM_IO_BASE + 0x204C00
#endif
};

#define NONE	{10000, 10000}

static unsigned s_GPIOConfig[DEVICES][GPIOS][VALUES] =
{
	// MISO     MOSI      SCLK     CE0      CE1
	{{ 9,  0}, {10,  0}, {11, 0}, { 8, 0}, { 7, 0}},
#if RASPPI >= 4
	{ NONE,     NONE,     NONE,    NONE,    NONE}, // unused
	{ NONE,     NONE,     NONE,    NONE,    NONE}, // unused
	{{ 1,  3}, { 2,  3}, { 3, 3}, { 0, 3}, {24, 5}},
	{{ 5,  3}, { 6,  3}, { 7, 3}, { 4, 3}, {25, 5}},
	{{13,  3}, {14,  3}, {15, 3}, {12, 3}, {26, 5}},
	{{19,  3}, {20,  3}, {21, 3}, {18, 3}, {27, 5}}
#endif
};

#define ALT_FUNC(device, gpio)	((TGPIOMode) (  s_GPIOConfig[device][gpio][VALUE_ALT] \
					      + GPIOModeAlternateFunction0))

CSPIMaster::CSPIMaster (unsigned nClockSpeed, unsigned CPOL, unsigned CPHA, unsigned nDevice)
:	m_nClockSpeed (nClockSpeed),
	m_CPOL (CPOL),
	m_CPHA (CPHA),
	m_nDevice (nDevice),
	m_nBaseAddress (0),
	m_bValid (FALSE),
	m_nCoreClockRate (CMachineInfo::Get ()->GetClockRate (CLOCK_ID_CORE)),
	m_nCSHoldTime (0),
	m_SpinLock (TASK_LEVEL)
{
	if (   m_nDevice >= DEVICES
	    || s_GPIOConfig[nDevice][0][VALUE_PIN] >= GPIO_PINS)
	{
		return;
	}

	m_nBaseAddress = s_BaseAddress[nDevice];
	assert (m_nBaseAddress != 0);

	m_MISO.AssignPin (s_GPIOConfig[nDevice][GPIO_MISO][VALUE_PIN]);
	m_MISO.SetMode (ALT_FUNC (nDevice, GPIO_MISO));

	m_MOSI.AssignPin (s_GPIOConfig[nDevice][GPIO_MOSI][VALUE_PIN]);
	m_MOSI.SetMode (ALT_FUNC (nDevice, GPIO_MOSI));

	m_SCLK.AssignPin (s_GPIOConfig[nDevice][GPIO_SCLK][VALUE_PIN]);
	m_SCLK.SetMode (ALT_FUNC (nDevice, GPIO_SCLK));

	m_CE0.AssignPin (s_GPIOConfig[nDevice][GPIO_CE0][VALUE_PIN]);
	m_CE0.SetMode (ALT_FUNC (nDevice, GPIO_CE0));

	m_CE1.AssignPin (s_GPIOConfig[nDevice][GPIO_CE1][VALUE_PIN]);
	m_CE1.SetMode (ALT_FUNC (nDevice, GPIO_CE1));

	assert (m_nCoreClockRate > 0);

	m_bValid = TRUE;
}

CSPIMaster::~CSPIMaster (void)
{
	if (m_bValid)
	{
		m_MISO.SetMode (GPIOModeInput);
		m_MOSI.SetMode (GPIOModeInput);
		m_SCLK.SetMode (GPIOModeInput);
		m_CE0.SetMode (GPIOModeInput);
		m_CE1.SetMode (GPIOModeInput);

		m_bValid = FALSE;
	}

	m_nBaseAddress = 0;
}

boolean CSPIMaster::Initialize (void)
{
	if (!m_bValid)
	{
		return FALSE;
	}

	assert (m_nBaseAddress != 0);

	PeripheralEntry ();

	assert (4000 <= m_nClockSpeed && m_nClockSpeed <= 125000000);
	write32 (ARM_SPI_CLK, m_nCoreClockRate / m_nClockSpeed);

	assert (m_CPOL <= 1);
	assert (m_CPHA <= 1);
	write32 (ARM_SPI_CS, (m_CPOL << CS_CPOL__SHIFT) | (m_CPHA << CS_CPHA__SHIFT));

	PeripheralExit ();

	return TRUE;
}

void CSPIMaster::SetClock (unsigned nClockSpeed)
{
	assert (m_bValid);
	assert (m_nBaseAddress != 0);

	m_nClockSpeed = nClockSpeed;

	PeripheralEntry ();

	assert (4000 <= m_nClockSpeed && m_nClockSpeed <= 125000000);
	write32 (ARM_SPI_CLK, m_nCoreClockRate / m_nClockSpeed);

	PeripheralExit ();
}

void CSPIMaster::SetMode (unsigned CPOL, unsigned CPHA)
{
	assert (m_bValid);
	assert (m_nBaseAddress != 0);

	m_CPOL = CPOL;
	m_CPHA = CPHA;

	PeripheralEntry ();

	assert (m_CPOL <= 1);
	assert (m_CPHA <= 1);
	write32 (ARM_SPI_CS, (m_CPOL << CS_CPOL__SHIFT) | (m_CPHA << CS_CPHA__SHIFT));

	PeripheralExit ();
}

void CSPIMaster::SetCSHoldTime (unsigned nMicroSeconds)
{
	assert (m_bValid);

	assert (nMicroSeconds < 200);
	m_nCSHoldTime = nMicroSeconds;
}

int CSPIMaster::Read (unsigned nChipSelect, void *pBuffer, unsigned nCount)
{
	return WriteRead (nChipSelect, 0, pBuffer, nCount);
}

int CSPIMaster::Write (unsigned nChipSelect, const void *pBuffer, unsigned nCount)
{
	return WriteRead (nChipSelect, pBuffer, 0, nCount);
}

int CSPIMaster::WriteRead (unsigned nChipSelect, const void *pWriteBuffer, void *pReadBuffer,
			   unsigned nCount)
{
	assert (m_bValid);
	assert (m_nBaseAddress != 0);

	assert (pWriteBuffer != 0 || pReadBuffer != 0);
	const u8 *pWritePtr = (const u8 *) pWriteBuffer;
	u8 *pReadPtr = (u8 *) pReadBuffer;

	m_SpinLock.Acquire ();

	PeripheralEntry ();

	// SCLK stays low for one clock cycle after each byte without this
	assert (nCount <= 0xFFFF);
	write32 (ARM_SPI_DLEN, nCount);

	assert (nChipSelect <= 1 || nChipSelect == ChipSelectNone);
	write32 (ARM_SPI_CS,   (read32 (ARM_SPI_CS) & ~CS_CS)
			     | (nChipSelect << CS_CS__SHIFT)
			     | CS_CLEAR_RX | CS_CLEAR_TX
			     | CS_TA);

	unsigned nWriteCount = 0;
	unsigned nReadCount = 0;

	assert (nCount > 0);
	while (   nWriteCount < nCount
	       || nReadCount  < nCount)
	{
		while (   nWriteCount < nCount
		       && (read32 (ARM_SPI_CS) & CS_TXD))
		{
			u32 nData = 0;
			if (pWritePtr != 0)
			{
				nData = *pWritePtr++;
			}

			write32 (ARM_SPI_FIFO, nData);

			nWriteCount++;
		}

		while (   nReadCount < nCount
		       && (read32 (ARM_SPI_CS) & CS_RXD))
		{
			u32 nData = read32 (ARM_SPI_FIFO);
			if (pReadPtr != 0)
			{
				*pReadPtr++ = (u8) nData;
			}

			nReadCount++;
		}
	}

	while (!(read32 (ARM_SPI_CS) & CS_DONE))
	{
		while (read32 (ARM_SPI_CS) & CS_RXD)
		{
			read32 (ARM_SPI_FIFO);
		}
	}

	if (m_nCSHoldTime > 0)
	{
		CTimer::Get ()->usDelay (m_nCSHoldTime);

		m_nCSHoldTime = 0;
	}

	write32 (ARM_SPI_CS, read32 (ARM_SPI_CS) & ~CS_TA);

	PeripheralExit ();

	m_SpinLock.Release ();

	return (int) nCount;
}
