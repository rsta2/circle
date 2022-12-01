//
// queue.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2021  R. Stange <rsta2@o2online.de>
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
#include "queue.h"
#include <assert.h>

CQueue::CQueue (unsigned nSize)
:	m_nSize (nSize),
	m_pBuffer (0),
	m_nInPtr (0),
	m_nOutPtr (0)
{
	assert (m_nSize > 1);

	m_pBuffer = new unsigned char[m_nSize];
	assert (m_pBuffer != 0);
}

CQueue::~CQueue (void)
{
	delete m_pBuffer;
	m_pBuffer = 0;
	
	m_nSize = 0;
}

boolean CQueue::IsEmpty (void) const
{
	return m_nOutPtr == m_nInPtr;
}

unsigned CQueue::GetFreeSpace (void) const
{
	assert (m_nSize > 1);
	assert (m_nInPtr < m_nSize);
	assert (m_nOutPtr < m_nSize);

	if (m_nOutPtr <= m_nInPtr)
	{
		return m_nSize+m_nOutPtr-m_nInPtr-1;
	}

	return m_nOutPtr-m_nInPtr-1;
}

void CQueue::Write (const void *pBuffer, unsigned nLength)
{
	assert (nLength > 0);
	assert (GetFreeSpace () >= nLength);

	unsigned char *p = (unsigned char *) pBuffer;
	assert (p != 0);
	assert (m_pBuffer != 0);

	while (nLength--)
	{
		m_pBuffer[m_nInPtr++] = *p++;
		m_nInPtr %= m_nSize;
	}
}

unsigned CQueue::GetBytesAvailable (void) const
{
	assert (m_nSize > 1);
	assert (m_nInPtr < m_nSize);
	assert (m_nOutPtr < m_nSize);

	if (m_nInPtr < m_nOutPtr)
	{
		return m_nSize+m_nInPtr-m_nOutPtr;
	}

	return m_nInPtr-m_nOutPtr;
}

void CQueue::Read (void *pBuffer, unsigned nLength)
{
	assert (nLength > 0);
	assert (GetBytesAvailable () >= nLength);

	unsigned char *p = (unsigned char *) pBuffer;
	assert (p != 0);
	assert (m_pBuffer != 0);

	while (nLength--)
	{
		*p++ = m_pBuffer[m_nOutPtr++];
		m_nOutPtr %= m_nSize;
	}
}

void CQueue::Flush (void)
{
	m_nInPtr = 0;
	m_nOutPtr = 0;
}
