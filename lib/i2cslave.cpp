//
// i2cslave.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2023  R. Stange <rsta2@o2online.de>
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
#include <circle/i2cslave.h>
#include <circle/memio.h>
#include <circle/bcm2835.h>
#include <circle/synchronize.h>
#include <circle/timer.h>
#include <assert.h>

#define DR_DATA__MASK		0xFF

#define RSR_UE			(1 << 1)
#define RSR_OE			(1 << 0)

#define CR_RXE			(1 << 9)
#define CR_TXE			(1 << 8)
#define CR_BRK			(1 << 7)
#define CR_I2C			(1 << 2)
#define CR_EN			(1 << 0)

#define FR_RXFLEVEL(reg)	(((reg) >> 11) & 0x1F)
#define FR_TXFLEVEL(reg)	(((reg) >> 6)  & 0x1F)
#define FR_RXBUSY		(1 << 5)
#define FR_TXFE			(1 << 4)
#define FR_RXFF			(1 << 3)
#define FR_TXFF			(1 << 2)
#define FR_RXFE			(1 << 1)
#define FR_TXBUSY		(1 << 0)

#define IFLS_RXIFLSEL__SHIFT	3
#define IFLS_TXIFLSEL__SHIFT	0

#define IMSC_OEIM		(1 << 3)
#define IMSC_BEIM		(1 << 2)
#define IMSC_TXIM		(1 << 1)
#define IMSC_RXIM		(1 << 0)

#define RIS_OERIS		(1 << 3)
#define RIS_BERIS		(1 << 2)
#define RIS_TXRIS		(1 << 1)
#define RIS_RXRIS		(1 << 0)

#define MIS_OEMIS		(1 << 3)
#define MIS_BEMIS		(1 << 2)
#define MIS_TXMIS		(1 << 1)
#define MIS_RXMIS		(1 << 0)

#define ICR_OEIC		(1 << 3)
#define ICR_BEIC		(1 << 2)
#define ICR_TXIC		(1 << 1)
#define ICR_RXIC		(1 << 0)

CI2CSlave::CI2CSlave (u8 ucAddress)
:	m_ucAddress (ucAddress),
#if RASPPI <= 3
	m_SDA (18, GPIOModeAlternateFunction3),
	m_SCL (19, GPIOModeAlternateFunction3)
#else
	m_SDA (10, GPIOModeAlternateFunction3),
	m_SCL (11, GPIOModeAlternateFunction3)
#endif
{
}

CI2CSlave::~CI2CSlave (void)
{
}

boolean CI2CSlave::Initialize (void)
{
	PeripheralEntry ();

	write32 (ARM_BSC_SPI_SLAVE_SLV, m_ucAddress);

	write32 (ARM_BSC_SPI_SLAVE_IMSC, 0);

	write32 (ARM_BSC_SPI_SLAVE_CR, CR_I2C | CR_EN);

	PeripheralExit ();

	return TRUE;
}

int CI2CSlave::Read (void *pBuffer, unsigned nCount, unsigned nTimeout_us)
{
	if (nCount == 0)
	{
		return -1;
	}

	u8 *pData = (u8 *) pBuffer;
	assert (pData != 0);

	PeripheralEntry ();

	write32 (ARM_BSC_SPI_SLAVE_RSR, 0);
	write32 (ARM_BSC_SPI_SLAVE_CR, read32 (ARM_BSC_SPI_SLAVE_CR) | CR_RXE);

	int nResult = 0;

	unsigned nTimeoutTicks = nTimeout_us * (CLOCKHZ / 1000000U);
	unsigned nStartTicks = CTimer::GetClockTicks ();

	while (nCount > 0)
	{
		while (read32 (ARM_BSC_SPI_SLAVE_FR) & FR_RXFE)
		{
			if (   nTimeout_us != TimeoutForEver
			    && CTimer::GetClockTicks () - nStartTicks >= nTimeoutTicks)
			{
				goto Timeout;
			}
		}

		if (read32 (ARM_BSC_SPI_SLAVE_RSR) & RSR_OE)
		{
			nResult = -1;

			break;
		}

		*pData++ = read32 (ARM_BSC_SPI_SLAVE_DR) & DR_DATA__MASK;

		nResult++;
		nCount--;
	}

Timeout:
	if (nResult > 0)
	{
		// wait for transfer to stop
		while (read32 (ARM_BSC_SPI_SLAVE_FR) & FR_RXBUSY)
		{
			if (read32 (ARM_BSC_SPI_SLAVE_RSR) & RSR_OE)
			{
				nResult = -1;

				break;
			}
		}
	}

	write32 (ARM_BSC_SPI_SLAVE_CR, read32 (ARM_BSC_SPI_SLAVE_CR) & ~CR_RXE);

	PeripheralExit ();

	return nResult;
}

int CI2CSlave::Write (const void *pBuffer, unsigned nCount, unsigned nTimeout_us)
{
	if (nCount == 0)
	{
		return -1;
	}

	const u8 *pData = (const u8 *) pBuffer;
	assert (pData != 0);

	PeripheralEntry ();

	write32 (ARM_BSC_SPI_SLAVE_RSR, 0);
	write32 (ARM_BSC_SPI_SLAVE_CR, read32 (ARM_BSC_SPI_SLAVE_CR) | CR_TXE);

	int nResult = 0;
	boolean bTxActive = FALSE;

	unsigned nTimeoutTicks = nTimeout_us * (CLOCKHZ / 1000000U);
	unsigned nStartTicks = CTimer::GetClockTicks ();

	while (nCount > 0)
	{
		if (read32 (ARM_BSC_SPI_SLAVE_FR) & FR_TXBUSY)
		{
			bTxActive = TRUE;
		}

		while (read32 (ARM_BSC_SPI_SLAVE_FR) & FR_TXFF)
		{
			if (   nTimeout_us != TimeoutForEver
			    && CTimer::GetClockTicks () - nStartTicks >= nTimeoutTicks)
			{
				goto Timeout;
			}
		}

		write32 (ARM_BSC_SPI_SLAVE_DR, *pData++);

		if (read32 (ARM_BSC_SPI_SLAVE_RSR) & RSR_UE)
		{
			nResult = -1;

			break;
		}

		nResult++;
		nCount--;
	}

Timeout:
	if (nResult > 0)
	{
		if (!bTxActive)
		{
			// wait for transfer to start
			while (!(read32 (ARM_BSC_SPI_SLAVE_FR) & FR_TXBUSY))
			{
				// do nothing
			}
		}

		// wait for transfer to stop
		while (read32 (ARM_BSC_SPI_SLAVE_FR) & FR_TXBUSY)
		{
			if (read32 (ARM_BSC_SPI_SLAVE_RSR) & RSR_UE)
			{
				nResult = -1;

				break;
			}
		}
	}

	write32 (ARM_BSC_SPI_SLAVE_CR, read32 (ARM_BSC_SPI_SLAVE_CR) & ~CR_TXE);

	PeripheralExit ();

	return nResult;
}
