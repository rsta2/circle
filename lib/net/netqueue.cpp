//
// netqueue.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2017  R. Stange <rsta2@o2online.de>
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
#include <circle/net/netqueue.h>
#include <circle/netdevice.h>
#include <circle/util.h>
#include <assert.h>

struct TNetQueueEntry
{
	volatile TNetQueueEntry *pPrev;
	volatile TNetQueueEntry *pNext;
	unsigned		 nLength;
	unsigned char		 Buffer[FRAME_BUFFER_SIZE];
	void			*pParam;
};

CNetQueue::CNetQueue (void)
:	m_pFirst (0),
	m_pLast (0),
	m_SpinLock (TASK_LEVEL)
{
}

CNetQueue::~CNetQueue (void)
{
	Flush ();
}

boolean CNetQueue::IsEmpty (void) const
{
	return m_pFirst == 0 ? TRUE : FALSE;
}

void CNetQueue::Flush (void)
{
	while (m_pFirst != 0)
	{
		m_SpinLock.Acquire ();

		volatile TNetQueueEntry *pEntry = m_pFirst;
		assert (pEntry != 0);

		m_pFirst = pEntry->pNext;
		if (m_pFirst != 0)
		{
			m_pFirst->pPrev = 0;
		}
		else
		{
			assert (m_pLast == pEntry);
			m_pLast = 0;
		}

		m_SpinLock.Release ();

		delete pEntry;
	}
}
	
void CNetQueue::Enqueue (const void *pBuffer, unsigned nLength, void *pParam)
{
	TNetQueueEntry *pEntry = new TNetQueueEntry;
	assert (pEntry != 0);

	assert (nLength > 0);
	assert (nLength <= FRAME_BUFFER_SIZE);
	pEntry->nLength = nLength;

	assert (pBuffer != 0);
	memcpy (pEntry->Buffer, pBuffer, nLength);

	pEntry->pParam = pParam;

	m_SpinLock.Acquire ();

	pEntry->pPrev = m_pLast;
	pEntry->pNext = 0;

	if (m_pFirst == 0)
	{
		m_pFirst = pEntry;
	}
	else
	{
		assert (m_pLast != 0);
		assert (m_pLast->pNext == 0);
		m_pLast->pNext = pEntry;
	}
	m_pLast = pEntry;

	m_SpinLock.Release ();
}

unsigned CNetQueue::Dequeue (void *pBuffer, void **ppParam)
{
	unsigned nResult = 0;
	
	if (m_pFirst != 0)
	{
		m_SpinLock.Acquire ();

		volatile TNetQueueEntry *pEntry = m_pFirst;
		assert (pEntry != 0);

		m_pFirst = pEntry->pNext;
		if (m_pFirst != 0)
		{
			m_pFirst->pPrev = 0;
		}
		else
		{
			assert (m_pLast == pEntry);
			m_pLast = 0;
		}

		m_SpinLock.Release ();

		nResult = pEntry->nLength;
		assert (nResult > 0);
		assert (nResult <= FRAME_BUFFER_SIZE);

		memcpy (pBuffer, (const void *) pEntry->Buffer, nResult);

		if (ppParam != 0)
		{
			*ppParam = pEntry->pParam;
		}

		delete pEntry;
	}

	return nResult;
}
