//
// logbuffer.cpp
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
#include <webconsole/logbuffer.h>
#include <assert.h>

CLogBuffer::CLogBuffer (unsigned nSize)
:	m_nSize (nSize),
	m_pBuffer (0),
	m_nInPtr (0),
	m_nOutPtr (0)
{
	m_pBuffer = new u8[m_nSize];
	assert (m_pBuffer != 0);
}

CLogBuffer::~CLogBuffer (void)
{
	delete m_pBuffer;
	m_pBuffer = 0;
}

void CLogBuffer::Put (const void *pBuffer, unsigned nLength)
{
	assert (m_pBuffer != 0);

	unsigned nFreeSpace = m_nOutPtr-m_nInPtr-1;	// may wrap
	if (nFreeSpace > m_nSize)
	{
		nFreeSpace += m_nSize;
	}

	if (nFreeSpace < nLength)
	{
		m_nOutPtr += nLength-nFreeSpace;
		m_nOutPtr %= m_nSize;

		while (m_nOutPtr != m_nInPtr)
		{
			if (m_pBuffer[m_nOutPtr] != '\n')
			{
				m_nOutPtr++;
				m_nOutPtr %= m_nSize;
			}
			else
			{
				m_nOutPtr++;
				m_nOutPtr %= m_nSize;

				break;
			}
		}
	}

	const u8 *p = (const u8 *) pBuffer;
	assert (p != 0);

	while (nLength-- > 0)
	{
		m_pBuffer[m_nInPtr++] = *p++;
		m_nInPtr %= m_nSize;
	}
}

unsigned CLogBuffer::Get (void *pBuffer)
{
	assert (m_pBuffer != 0);

	unsigned nResult = m_nInPtr-m_nOutPtr;		// may wrap
	if (nResult > m_nSize)
	{
		nResult += m_nSize;
	}

	u8 *p = (u8 *) pBuffer;
	assert (p != 0);

	unsigned nPreOutPtr = m_nOutPtr;
	for (unsigned nLength = nResult; nLength > 0; nLength--)
	{
		*p++ = m_pBuffer[nPreOutPtr++];
		nPreOutPtr %= m_nSize;
	}

	return nResult;
}
