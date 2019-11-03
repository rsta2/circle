//
// spimasteraux.cpp
//
// This driver has been ported from:
//	https://github.com/vanvught/rpidmx512/blob/master/lib-bcm2835/src/bcm2835_aux_spi.c
//
// Copyright (C) 2017 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include <circle/spimasteraux.h>
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/machineinfo.h>
#include <circle/synchronize.h>
#include <assert.h>

#define AUX_SPI_CNTL0_SPEED		0xFFF00000
	#define AUX_SPI_CNTL0_SPEED_MAX		0xFFF
	#define AUX_SPI_CNTL0_SPEED_SHIFT	20

#define AUX_SPI_CNTL0_CS0_N     	0x000C0000	// CS 0 low
#define AUX_SPI_CNTL0_CS1_N     	0x000A0000	// CS 1 low
#define AUX_SPI_CNTL0_CS2_N 		0x00060000	// CS 2 low

#define AUX_SPI_CNTL0_POSTINPUT		0x00010000
#define AUX_SPI_CNTL0_VAR_CS		0x00008000
#define AUX_SPI_CNTL0_VAR_WIDTH		0x00004000
#define AUX_SPI_CNTL0_DOUTHOLD		0x00003000
#define AUX_SPI_CNTL0_ENABLE		0x00000800
#define AUX_SPI_CNTL0_CPHA_IN		0x00000400
#define AUX_SPI_CNTL0_CLEARFIFO		0x00000200
#define AUX_SPI_CNTL0_CPHA_OUT		0x00000100
#define AUX_SPI_CNTL0_CPOL		0x00000080
#define AUX_SPI_CNTL0_MSBF_OUT		0x00000040
#define AUX_SPI_CNTL0_SHIFTLEN		0x0000003F

#define AUX_SPI_CNTL1_CSHIGH		0x00000700
#define AUX_SPI_CNTL1_IDLE		0x00000080
#define AUX_SPI_CNTL1_TXEMPTY		0x00000040
#define AUX_SPI_CNTL1_MSBF_IN		0x00000002
#define AUX_SPI_CNTL1_KEEP_IN		0x00000001

#define AUX_SPI_STAT_TX_LVL		0xFF000000
#define AUX_SPI_STAT_RX_LVL		0x00FF0000
#define AUX_SPI_STAT_TX_FULL		0x00000400
#define AUX_SPI_STAT_TX_EMPTY		0x00000200
#define AUX_SPI_STAT_RX_FULL		0x00000100
#define AUX_SPI_STAT_RX_EMPTY		0x00000080
#define AUX_SPI_STAT_BUSY		0x00000040
#define AUX_SPI_STAT_BITCOUNT		0x0000003F

#define DIV_ROUND_UP(n,d)		(((n) + (d) - 1) / (d))
#define MIN(a,b)			(((a) < (b)) ? (a) : (b))

CSPIMasterAUX::CSPIMasterAUX (unsigned nClockSpeed)
:	m_nClockSpeed (nClockSpeed),
	m_SCLK (21, GPIOModeAlternateFunction4),
	m_MOSI (20, GPIOModeAlternateFunction4),
	m_MISO (19, GPIOModeAlternateFunction4),
	m_CE0  (18, GPIOModeAlternateFunction4),
	m_CE1  (17, GPIOModeAlternateFunction4),
	m_CE2  (16, GPIOModeAlternateFunction4),
	m_nCoreClockRate (CMachineInfo::Get ()->GetClockRate (CLOCK_ID_CORE)),
	m_nClockDivider (0),
	m_SpinLock (TASK_LEVEL)
{
	assert (m_nCoreClockRate > 0);
}

CSPIMasterAUX::~CSPIMasterAUX (void)
{
	PeripheralEntry ();

	write32 (ARM_AUX_ENABLE, read32 (ARM_AUX_ENABLE) & ~ARM_AUX_ENABLE_SPI1);

	PeripheralExit ();
}

boolean CSPIMasterAUX::Initialize (void)
{
	SetClock (m_nClockSpeed);

	PeripheralEntry ();

	write32 (ARM_AUX_ENABLE, read32 (ARM_AUX_ENABLE) | ARM_AUX_ENABLE_SPI1);

	write32 (ARM_AUX_SPI0_CNTL1, 0);
	write32 (ARM_AUX_SPI0_CNTL0, AUX_SPI_CNTL0_CLEARFIFO);

	PeripheralExit ();

	return TRUE;
}

void CSPIMasterAUX::SetClock (unsigned nClockSpeed)
{
	m_nClockSpeed = nClockSpeed;

	assert (30500 <= m_nClockSpeed && m_nClockSpeed <= 125000000);

	m_nClockDivider = DIV_ROUND_UP (m_nCoreClockRate, 2 * nClockSpeed) - 1;
	if (m_nClockDivider > AUX_SPI_CNTL0_SPEED_MAX)
	{
		m_nClockDivider = AUX_SPI_CNTL0_SPEED_MAX;
	}
}

int CSPIMasterAUX::Read (unsigned nChipSelect, void *pBuffer, unsigned nCount)
{
	return WriteRead (nChipSelect, 0, pBuffer, nCount);
}

int CSPIMasterAUX::Write (unsigned nChipSelect, const void *pBuffer, unsigned nCount)
{
	return WriteRead (nChipSelect, pBuffer, 0, nCount);
}

int CSPIMasterAUX::WriteRead (unsigned nChipSelect,
			      const void *pWriteBuffer, void *pReadBuffer, unsigned nCount)
{
	assert (pWriteBuffer != 0 || pReadBuffer != 0);
	const u8 *pWritePtr = (const u8 *) pWriteBuffer;
	u8 *pReadPtr = (u8 *) pReadBuffer;

	assert (nCount > 0);
	unsigned nWriteCount = nCount;
	unsigned nReadCount = nCount;

	m_SpinLock.Acquire ();

	PeripheralEntry ();

	u32 nCntl0 = (m_nClockDivider << AUX_SPI_CNTL0_SPEED_SHIFT);

	switch (nChipSelect)
	{
	case 0:
		nCntl0 |= AUX_SPI_CNTL0_CS0_N;
		break;

	case 1:
		nCntl0 |= AUX_SPI_CNTL0_CS1_N;
		break;

	case 2:
		nCntl0 |= AUX_SPI_CNTL0_CS2_N;
		break;

	default:
		assert (0);
		break;
	}

	nCntl0 |= AUX_SPI_CNTL0_ENABLE;
	nCntl0 |= AUX_SPI_CNTL0_MSBF_OUT;
	nCntl0 |= AUX_SPI_CNTL0_VAR_WIDTH;
	write32 (ARM_AUX_SPI0_CNTL0, nCntl0);

	write32 (ARM_AUX_SPI0_CNTL1, AUX_SPI_CNTL1_MSBF_IN);

	while ((nWriteCount > 0) || (nReadCount > 0))
	{
		while (!(read32 (ARM_AUX_SPI0_STAT) & AUX_SPI_STAT_TX_FULL) && (nWriteCount > 0))
		{
			u32 nBytes = MIN (nWriteCount, 3);
			u32 nData = 0;

			for (unsigned i = 0; i < nBytes; i++)
			{
				u8 nByte = (pWritePtr != 0) ? *pWritePtr++ : 0;
				nData |= nByte << (8 * (2 - i));
			}

			nData |= (nBytes * 8) << 24;
			nWriteCount -= nBytes;

			if (nWriteCount != 0)
			{
				write32 (ARM_AUX_SPI0_TXHOLD, nData);
			}
			else
			{
				write32 (ARM_AUX_SPI0_IO, nData);
			}
		}

		while (!(read32 (ARM_AUX_SPI0_STAT) & AUX_SPI_STAT_RX_EMPTY) && (nReadCount > 0))
		{
			u32 nBytes = MIN (nReadCount, 3);
			u32 nData = read32 (ARM_AUX_SPI0_IO);

			if (pReadBuffer != 0)
			{
				switch (nBytes)
				{
				case 3:
					*pReadPtr++ = (u8) ((nData >> 16) & 0xFF);
					// fall through

				case 2:
					*pReadPtr++ = (u8) ((nData >> 8) & 0xFF);
					// fall through

				case 1:
					*pReadPtr++ = (u8) ((nData >> 0) & 0xFF);
				}
			}

			nReadCount -= nBytes;
		}

		while (!(read32 (ARM_AUX_SPI0_STAT) & AUX_SPI_STAT_BUSY) && (nReadCount > 0))
		{
			u32 nBytes = MIN (nReadCount, 3);
			u32 nData = read32 (ARM_AUX_SPI0_IO);

			if (pReadBuffer != 0)
			{
				switch (nBytes)
				{
				case 3:
					*pReadPtr++ = (u8) ((nData >> 16) & 0xFF);
					// fall through

				case 2:
					*pReadPtr++ = (u8) ((nData >> 8) & 0xFF);
					// fall through

				case 1:
					*pReadPtr++ = (u8) ((nData >> 0) & 0xFF);
				}
			}

			nReadCount -= nBytes;
		}
	}

	PeripheralExit ();

	m_SpinLock.Release ();

	return (int) nCount;
}
