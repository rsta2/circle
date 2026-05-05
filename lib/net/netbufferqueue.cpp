//
// netbufferqueue.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2025-2026  R. Stange <rsta2@gmx.net>
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
#include <circle/net/netbufferqueue.h>
#include <assert.h>

CNetBufferQueue::CNetBufferQueue (boolean bProtected)
:	m_pFirst (nullptr),
	m_pLast (nullptr),
	m_nEntries (0),
	m_ulBytes (0),
	m_pPeekHead (nullptr),
	m_bProtected (bProtected),
	m_SpinLock (TASK_LEVEL)
{
}

CNetBufferQueue::~CNetBufferQueue (void)
{
	Flush ();
}

boolean CNetBufferQueue::IsEmpty (void) const
{
	return !m_pFirst;
}

unsigned CNetBufferQueue::GetNumEntries (void) const
{
	return m_nEntries;
}

size_t CNetBufferQueue::GetBytesQueued (void) const
{
	return m_ulBytes;
}

void CNetBufferQueue::Flush (size_t ulBytes)
{
	while (m_pFirst && ulBytes)
	{
		if (m_bProtected)
		{
			m_SpinLock.Acquire ();
		}

		CNetBuffer *pEntry = m_pFirst;

		size_t ulLength = pEntry->GetLength ();
		if (ulLength > ulBytes)
		{
			if (m_bProtected)
			{
				m_SpinLock.Release ();
			}

			break;
		}

		ulBytes -= ulLength;

		m_pFirst = pEntry->m_pNext;
		if (!m_pFirst)
		{
			assert (m_pLast == pEntry);
			m_pLast = nullptr;
		}

		pEntry->m_pNext = nullptr;

		assert (m_nEntries);
		m_nEntries--;

		assert (m_ulBytes >= ulLength);
		m_ulBytes -= ulLength;

		if (m_pPeekHead == pEntry)
		{
			m_pPeekHead = m_pFirst;
		}

		if (m_bProtected)
		{
			m_SpinLock.Release ();
		}

		delete pEntry;
	}
}

void CNetBufferQueue::Enqueue (CNetBuffer *pNetBuffer)
{
	assert (pNetBuffer);
	assert (!pNetBuffer->m_pNext);

	size_t ulLength = pNetBuffer->GetLength ();

	if (m_bProtected)
	{
		m_SpinLock.Acquire ();
	}

	if (!m_pFirst)
	{
		m_pFirst = pNetBuffer;
	}
	else
	{
		assert (m_pLast);
		assert (!m_pLast->m_pNext);
		m_pLast->m_pNext = pNetBuffer;
	}

	m_pLast = pNetBuffer;

	m_nEntries++;
	m_ulBytes += ulLength;

	if (!m_pPeekHead)
	{
		m_pPeekHead = pNetBuffer;
	}

	if (m_bProtected)
	{
		m_SpinLock.Release ();
	}
}

CNetBuffer *CNetBufferQueue::Dequeue (void)
{
	CNetBuffer *pEntry = m_pFirst;
	if (pEntry)
	{
		if (m_bProtected)
		{
			m_SpinLock.Acquire ();
		}

		m_pFirst = pEntry->m_pNext;

		if (!m_pFirst)
		{
			m_pLast = nullptr;
		}

		pEntry->m_pNext = nullptr;

		assert (m_nEntries);
		m_nEntries--;

		size_t ulLength = pEntry->GetLength ();
		assert (m_ulBytes >= ulLength);
		m_ulBytes -= ulLength;

		if (m_pPeekHead == pEntry)
		{
			m_pPeekHead = m_pFirst;
		}

		if (m_bProtected)
		{
			m_SpinLock.Release ();
		}
	}

	return pEntry;
}

const CNetBuffer *CNetBufferQueue::PeekFirst (void) const
{
	return m_pFirst;
}

const CNetBuffer *CNetBufferQueue::Peek (void) const
{
	return m_pPeekHead;
}

void CNetBufferQueue::Rewind (void)
{
	m_pPeekHead = m_pFirst;
}

void CNetBufferQueue::MoveOn (void)
{
	if (m_bProtected)
	{
		m_SpinLock.Acquire ();
	}

	if (m_pPeekHead)
	{
		m_pPeekHead = m_pPeekHead->m_pNext;
	}

	if (m_bProtected)
	{
		m_SpinLock.Release ();
	}
}
