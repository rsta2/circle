//
// spimaster.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>
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
#define CS_CS		(1 << 0)
#define CS_CS__SHIFT	0

CSPIMaster::CSPIMaster (unsigned nClockSpeed, unsigned CPOL, unsigned CPHA)
:	m_nClockSpeed (nClockSpeed),
	m_CPOL (CPOL),
	m_CPHA (CPHA),
	m_SCLK (11, GPIOModeAlternateFunction0),
	m_MOSI (10, GPIOModeAlternateFunction0),
	m_MISO ( 9, GPIOModeAlternateFunction0),
	m_CE0  ( 8, GPIOModeAlternateFunction0),
	m_CE1  ( 7, GPIOModeAlternateFunction0),
	m_nCoreClockRate (CMachineInfo::Get ()->GetClockRate (CLOCK_ID_CORE)),
	m_nCSHoldTime (0),
	m_SpinLock (FALSE)
{
	assert (m_nCoreClockRate > 0);
}

CSPIMaster::~CSPIMaster (void)
{
}

boolean CSPIMaster::Initialize (void)
{
	PeripheralEntry ();

	assert (4000 <= m_nClockSpeed && m_nClockSpeed <= 125000000);
	write32 (ARM_SPI0_CLK, m_nCoreClockRate / m_nClockSpeed);

	assert (m_CPOL <= 1);
	assert (m_CPHA <= 1);
	write32 (ARM_SPI0_CS, (m_CPOL << CS_CPOL__SHIFT) | (m_CPHA << CS_CPHA__SHIFT));

	PeripheralExit ();

	return TRUE;
}

void CSPIMaster::SetClock (unsigned nClockSpeed)
{
	m_nClockSpeed = nClockSpeed;

	PeripheralEntry ();

	assert (4000 <= m_nClockSpeed && m_nClockSpeed <= 125000000);
	write32 (ARM_SPI0_CLK, m_nCoreClockRate / m_nClockSpeed);

	PeripheralExit ();
}

void CSPIMaster::SetMode (unsigned CPOL, unsigned CPHA)
{
	m_CPOL = CPOL;
	m_CPHA = CPHA;

	PeripheralEntry ();

	assert (m_CPOL <= 1);
	assert (m_CPHA <= 1);
	write32 (ARM_SPI0_CS, (m_CPOL << CS_CPOL__SHIFT) | (m_CPHA << CS_CPHA__SHIFT));

	PeripheralExit ();
}

void CSPIMaster::SetCSHoldTime (unsigned nMicroSeconds)
{
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

int CSPIMaster::WriteRead (unsigned nChipSelect, const void *pWriteBuffer, void *pReadBuffer, unsigned nCount)
{
	assert (pWriteBuffer != 0 || pReadBuffer != 0);
	const u8 *pWritePtr = (const u8 *) pWriteBuffer;
	u8 *pReadPtr = (u8 *) pReadBuffer;

	m_SpinLock.Acquire ();

	PeripheralEntry ();

	assert (nChipSelect <= 1);
	write32 (ARM_SPI0_CS,   (read32 (ARM_SPI0_CS) & ~CS_CS)
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
		       && (read32 (ARM_SPI0_CS) & CS_TXD))
		{
			u32 nData = 0;
			if (pWritePtr != 0)
			{
				nData = *pWritePtr++;
			}

			write32 (ARM_SPI0_FIFO, nData);

			nWriteCount++;
		}

		while (   nReadCount < nCount
		       && (read32 (ARM_SPI0_CS) & CS_RXD))
		{
			u32 nData = read32 (ARM_SPI0_FIFO);
			if (pReadPtr != 0)
			{
				*pReadPtr++ = (u8) nData;
			}

			nReadCount++;
		}
	}

	while (!(read32 (ARM_SPI0_CS) & CS_DONE))
	{
		while (read32 (ARM_SPI0_CS) & CS_RXD)
		{
			read32 (ARM_SPI0_FIFO);
		}
	}

	if (m_nCSHoldTime > 0)
	{
		CTimer::Get ()->usDelay (m_nCSHoldTime);

		m_nCSHoldTime = 0;
	}

	write32 (ARM_SPI0_CS, read32 (ARM_SPI0_CS) & ~CS_TA);

	PeripheralExit ();

	m_SpinLock.Release ();

	return (int) nCount;
}
