//
// dwhcixactqueue.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2022  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/dwhcixactqueue.h>
#include <circle/usb/usbrequest.h>
#include <circle/usb/dwhci.h>
#include <circle/classallocator.h>
#include <assert.h>

#ifdef USE_USB_SOF_INTR

struct TQueueEntry
{
#ifndef NDEBUG
	unsigned		 nMagic;
#define XACT_QUEUE_MAGIC	0x58415055
#endif
	CDWHCITransferStageData *pStageData;
	u16			 usFrameNumber;

	DECLARE_CLASS_ALLOCATOR
};

IMPLEMENT_CLASS_ALLOCATOR (TQueueEntry)

// Returns TRUE if usFrameNumber1 is greater than usFrameNumber2
static inline boolean FrameNumberGreater (u16 usFrameNumber1, u16 usFrameNumber2)
{
	if (usFrameNumber1 == usFrameNumber2)
	{
		return FALSE;
	}

	if (   ((usFrameNumber1 - usFrameNumber2) & DWHCI_MAX_FRAME_NUMBER)
	    >= (DWHCI_MAX_FRAME_NUMBER >> 1))
	{
		return FALSE;
	}

	return TRUE;
}

CDWHCITransactionQueue::CDWHCITransactionQueue (unsigned nMaxElements, unsigned nMaxAccessLevel)
:	m_List (nMaxElements),
	m_SpinLock (nMaxAccessLevel)
{
	INIT_PROTECTED_CLASS_ALLOCATOR (TQueueEntry, nMaxElements, nMaxAccessLevel);
}

CDWHCITransactionQueue::~CDWHCITransactionQueue (void)
{
	Flush ();
}

void CDWHCITransactionQueue::Flush (void)
{
	m_SpinLock.Acquire ();

	TPtrListElement *pElement = m_List.GetFirst ();
	while (pElement != 0)
	{
		TQueueEntry *pEntry = (TQueueEntry *) m_List.GetPtr (pElement);
		assert (pEntry != 0);
		assert (pEntry->nMagic == XACT_QUEUE_MAGIC);

		m_List.Remove (pElement);

		assert (pEntry->pStageData != 0);
		CUSBRequest *pURB = pEntry->pStageData->GetURB ();
		delete pURB;

		delete pEntry->pStageData;

#ifndef NDEBUG
		pEntry->nMagic = 0;
#endif
		delete pEntry;

		pElement = m_List.GetFirst ();
	}

	m_SpinLock.Release ();
}

void CDWHCITransactionQueue::FlushDevice (CUSBDevice *pUSBDevice)
{
	assert (pUSBDevice != 0);

	m_SpinLock.Acquire ();

	TPtrListElement *pElement = m_List.GetFirst ();
	while (pElement != 0)
	{
		TQueueEntry *pEntry = (TQueueEntry *) m_List.GetPtr (pElement);
		assert (pEntry != 0);
		assert (pEntry->nMagic == XACT_QUEUE_MAGIC);

		TPtrListElement *pNextElement = m_List.GetNext (pElement);

		assert (pEntry->pStageData != 0);
		if (pEntry->pStageData->GetDevice () == pUSBDevice)
		{
			m_List.Remove (pElement);

			assert (pEntry->pStageData != 0);
			CUSBRequest *pURB = pEntry->pStageData->GetURB ();
			delete pURB;

			delete pEntry->pStageData;

#ifndef NDEBUG
			pEntry->nMagic = 0;
#endif
			delete pEntry;
		}

		pElement = pNextElement;
	}

	m_SpinLock.Release ();
}

void CDWHCITransactionQueue::Enqueue (CDWHCITransferStageData *pStageData, u16 usFrameNumber)
{
	assert (pStageData != 0);
	assert (usFrameNumber <= DWHCI_MAX_FRAME_NUMBER);

	TQueueEntry *pEntry = new TQueueEntry;
	assert (pEntry != 0);
#ifndef NDEBUG
	pEntry->nMagic        = XACT_QUEUE_MAGIC;
#endif
	pEntry->pStageData    = pStageData;
	pEntry->usFrameNumber = usFrameNumber;

	m_SpinLock.Acquire ();

	TPtrListElement *pPrevElement = 0;
	TPtrListElement *pElement = m_List.GetFirst ();
	while (pElement != 0)
	{
		TQueueEntry *pEntry2 = (TQueueEntry *) m_List.GetPtr (pElement);
		assert (pEntry2 != 0);
		assert (pEntry2->nMagic == XACT_QUEUE_MAGIC);

		if (FrameNumberGreater (pEntry2->usFrameNumber, usFrameNumber))
		{
			break;
		}

		pPrevElement = pElement;
		pElement = m_List.GetNext (pElement);
	}

	if (pElement != 0)
	{
		m_List.InsertBefore (pElement, pEntry);
	}
	else
	{
		m_List.InsertAfter (pPrevElement, pEntry);
	}

	m_SpinLock.Release ();
}

CDWHCITransferStageData *CDWHCITransactionQueue::Dequeue (u16 usFrameNumber)
{
	m_SpinLock.Acquire ();

	TPtrListElement *pElement = m_List.GetFirst ();
	if (pElement == 0)
	{
		m_SpinLock.Release ();

		return 0;
	}

	TQueueEntry *pEntry = (TQueueEntry *) m_List.GetPtr (pElement);
	assert (pEntry != 0);
	assert (pEntry->nMagic == XACT_QUEUE_MAGIC);

	if (FrameNumberGreater (pEntry->usFrameNumber, usFrameNumber))
	{
		m_SpinLock.Release ();

		return 0;
	}

	m_List.Remove (pElement);

	m_SpinLock.Release ();

	CDWHCITransferStageData *pStageData = pEntry->pStageData;
	assert (pStageData != 0);

#ifndef NDEBUG
	pEntry->nMagic = 0;
#endif
	delete pEntry;

	return pStageData;
}

#endif
