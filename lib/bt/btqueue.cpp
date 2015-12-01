//
// btqueue.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015  R. Stange <rsta2@o2online.de>
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
#include <circle/bt/btqueue.h>
#include <circle/bt/bluetooth.h>
#include <circle/util.h>
#include <assert.h>

struct TBTQueueEntry
{
	volatile TBTQueueEntry	*pPrev;
	volatile TBTQueueEntry	*pNext;
	unsigned		 nLength;
	unsigned char		 Buffer[BT_MAX_DATA_SIZE];
	void			*pParam;
};

CBTQueue::CBTQueue (void)
:	m_pFirst (0),
	m_pLast (0)
{
}

CBTQueue::~CBTQueue (void)
{
	Flush ();
}

boolean CBTQueue::IsEmpty (void) const
{
	return m_pFirst == 0 ? TRUE : FALSE;
}

void CBTQueue::Flush (void)
{
	while (m_pFirst != 0)
	{
		m_SpinLock.Acquire ();

		volatile TBTQueueEntry *pEntry = m_pFirst;
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
	
void CBTQueue::Enqueue (const void *pBuffer, unsigned nLength, void *pParam)
{
	TBTQueueEntry *pEntry = new TBTQueueEntry;
	assert (pEntry != 0);

	assert (nLength > 0);
	assert (nLength <= BT_MAX_DATA_SIZE);
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

unsigned CBTQueue::Dequeue (void *pBuffer, void **ppParam)
{
	unsigned nResult = 0;
	
	if (m_pFirst != 0)
	{
		m_SpinLock.Acquire ();

		volatile TBTQueueEntry *pEntry = m_pFirst;
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
		assert (nResult <= BT_MAX_DATA_SIZE);

		memcpy (pBuffer, (const void *) pEntry->Buffer, nResult);

		if (ppParam != 0)
		{
			*ppParam = pEntry->pParam;
		}

		delete pEntry;
	}

	return nResult;
}
