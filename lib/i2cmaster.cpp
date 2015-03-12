//
// i2cmaster.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2015  R. Stange <rsta2@o2online.de>
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
#include <circle/synchronize.h>
#include <assert.h>

// Control register
#define C_I2CEN			(1 << 15)
#define C_INTR			(1 << 10)
#define C_INTT			(1 << 9)
#define C_INTD			(1 << 8)
#define C_ST			(1 << 7)
#define C_CLEAR			(1 << 4)
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

CI2CMaster::CI2CMaster (unsigned nDevice, boolean bFastMode)
:	m_nDevice (nDevice),
	m_nBaseAddress (nDevice == 0 ? ARM_BSC0_BASE : ARM_BSC1_BASE),
	m_bFastMode (bFastMode),
	m_SDA (nDevice == 0 ? 0 : 2, GPIOModeAlternateFunction0),
	m_SCL (nDevice == 0 ? 1 : 3, GPIOModeAlternateFunction0),
	m_SpinLock (FALSE)
{
	assert (nDevice <= 1);
}

CI2CMaster::~CI2CMaster (void)
{
	m_nBaseAddress = 0;
}

boolean CI2CMaster::Initialize (void)
{
	DataMemBarrier ();

	u32 nDivider = m_bFastMode ? 150000 / 400 : 150000 / 100;
	write32 (m_nBaseAddress + ARM_BSC_DIV__OFFSET, nDivider);
	
	write32 (m_nBaseAddress + ARM_BSC_C__OFFSET, C_I2CEN);

	DataMemBarrier ();

	return TRUE;
}

int CI2CMaster::Read (u8 ucAddress, void *pBuffer, unsigned nCount)
{
	if (ucAddress >= 0x80)
	{
		return -1;
	}

	if (nCount == 0)
	{
		return -1;
	}

	m_SpinLock.Acquire ();

	u8 *pData = (u8 *) pBuffer;
	assert (pData != 0);

	int nResult = 0;

	DataMemBarrier ();

	write32 (m_nBaseAddress + ARM_BSC_A__OFFSET, ucAddress);
	write32 (m_nBaseAddress + ARM_BSC_DLEN__OFFSET, nCount);

	write32 (m_nBaseAddress + ARM_BSC_S__OFFSET, S_CLKT | S_ERR | S_DONE);
	write32 (m_nBaseAddress + ARM_BSC_C__OFFSET,   read32 (m_nBaseAddress + ARM_BSC_C__OFFSET)
						     | C_ST | C_READ | C_CLEAR);

	// wait for transfer to start
	u32 nStatus;
	while (!((nStatus = read32 (m_nBaseAddress + ARM_BSC_S__OFFSET)) & S_TA))
	{
		if (nStatus & (S_CLKT | S_ERR))
		{
			DataMemBarrier ();

			m_SpinLock.Release ();

			return -1;
		}
	}

	// transfer active
	while (nCount-- > 0)
	{
		while (!((nStatus = read32 (m_nBaseAddress + ARM_BSC_S__OFFSET)) & S_RXD))
		{
			if (nStatus & (S_CLKT | S_ERR))
			{
				DataMemBarrier ();

				m_SpinLock.Release ();

				return -1;
			}
		}

		*pData++ = read32 (m_nBaseAddress + ARM_BSC_FIFO__OFFSET) & FIFO__MASK;

		nResult++;
	}

	// wait for transfer to stop
	while ((nStatus = read32 (m_nBaseAddress + ARM_BSC_S__OFFSET)) & S_TA)
	{
		if (nStatus & (S_CLKT | S_ERR))
		{
			DataMemBarrier ();

			m_SpinLock.Release ();

			return -1;
		}
	}

	DataMemBarrier ();

	m_SpinLock.Release ();

	return nResult;
}

int CI2CMaster::Write (u8 ucAddress, const void *pBuffer, unsigned nCount)
{
	if (ucAddress >= 0x80)
	{
		return -1;
	}

	if (nCount == 0)
	{
		return -1;
	}

	m_SpinLock.Acquire ();

	u8 *pData = (u8 *) pBuffer;
	assert (pData != 0);

	int nResult = 0;

	DataMemBarrier ();

	write32 (m_nBaseAddress + ARM_BSC_A__OFFSET, ucAddress);
	write32 (m_nBaseAddress + ARM_BSC_DLEN__OFFSET, nCount);

	write32 (m_nBaseAddress + ARM_BSC_S__OFFSET, S_CLKT | S_ERR | S_DONE);
	write32 (m_nBaseAddress + ARM_BSC_C__OFFSET,  (read32 (m_nBaseAddress + ARM_BSC_C__OFFSET) & ~C_READ)
						     | C_ST | C_CLEAR);

	// wait for transfer to start
	u32 nStatus;
	while (!((nStatus = read32 (m_nBaseAddress + ARM_BSC_S__OFFSET)) & S_TA))
	{
		if (nStatus & (S_CLKT | S_ERR))
		{
			DataMemBarrier ();

			m_SpinLock.Release ();

			return -1;
		}
	}

	// transfer active
	while (nCount-- > 0)
	{
		while (!((nStatus = read32 (m_nBaseAddress + ARM_BSC_S__OFFSET)) & S_TXD))
		{
			if (nStatus & (S_CLKT | S_ERR))
			{
				DataMemBarrier ();

				m_SpinLock.Release ();

				return -1;
			}
		}

		write32 (m_nBaseAddress + ARM_BSC_FIFO__OFFSET, *pData++);

		nResult++;
	}

	// wait for transfer to stop
	while ((nStatus = read32 (m_nBaseAddress + ARM_BSC_S__OFFSET)) & S_TA)
	{
		if (nStatus & (S_CLKT | S_ERR))
		{
			DataMemBarrier ();

			m_SpinLock.Release ();

			return -1;
		}
	}

	DataMemBarrier ();

	m_SpinLock.Release ();

	return nResult;
}
