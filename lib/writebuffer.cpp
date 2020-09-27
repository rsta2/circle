//
// writebuffer.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020  R. Stange <rsta2@o2online.de>
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
#include <circle/writebuffer.h>
#include <circle/synchronize.h>
#include <circle/macros.h>
#include <circle/new.h>
#include <assert.h>

CWriteBufferDevice::CWriteBufferDevice (CDevice *pDevice, size_t nBufferSize)
:	m_pDevice (pDevice),
	m_nBufferSize (nBufferSize),
	m_nInPtr (0),
	m_nOutPtr (0)
{
	assert (m_pDevice != 0);
	assert (IS_POWEROF_2 (m_nBufferSize));

	m_pBuffer = new (HEAP_ANY) u8[m_nBufferSize];
	assert (m_pBuffer != 0);
}

CWriteBufferDevice::~CWriteBufferDevice (void)
{
	delete [] m_pBuffer;
	m_pBuffer = 0;

	m_pDevice = 0;
}

int CWriteBufferDevice::Write (const void *pBuffer, size_t nCount)
{
	assert (pBuffer != 0);
	assert (m_pBuffer != 0);

	u8 *p = (u8 *) pBuffer;

	int nResult = 0;

	m_SpinLock.Acquire ();

	while (nCount-- > 0)
	{
		if (((m_nInPtr+1) & (m_nBufferSize-1)) == m_nOutPtr)
		{
			break;
		}

		m_pBuffer[m_nInPtr++] = *p++;
		m_nInPtr &= m_nBufferSize-1;

		nResult++;
	}

	m_SpinLock.Release ();

	return nResult;
}

void CWriteBufferDevice::Update (size_t nMaxBytes)
{
	assert (m_pBuffer != 0);

	DMA_BUFFER (u8, Buffer, nMaxBytes);
	u8 *p = Buffer;

	m_SpinLock.Acquire ();

	unsigned nBytes;
	for (nBytes = 0; nBytes < nMaxBytes; nBytes++)
	{
		if (m_nInPtr == m_nOutPtr)
		{
			break;
		}

		*p++ = m_pBuffer[m_nOutPtr++];
		m_nOutPtr &= m_nBufferSize-1;
	}

	m_SpinLock.Release ();

	if (nBytes > 0)
	{
		assert (m_pDevice != 0);
		m_pDevice->Write (Buffer, nBytes);
	}
}
