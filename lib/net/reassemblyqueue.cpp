//
// reassemblyqueue.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2025  R. Stange <rsta2@gmx.net>
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
#include <circle/net/reassemblyqueue.h>
#include <circle/logger.h>
#include <assert.h>

LOGMODULE ("tcpraq");

// Modulo 32 sequence number arithmetic
static inline boolean lt (u32 x, u32 y)
{
	return static_cast<int> (x - y) < 0;
}

static inline boolean le (u32 x, u32 y)
{
	return static_cast<int> (x - y) <= 0;
}

CReassemblyQueue::CReassemblyQueue (CNetBufferQueue *pRxQueue, size_t ulMaxBytes)
:	m_pRxQueue (pRxQueue),
	m_ulMaxBytes (ulMaxBytes),
	m_ulCurrentBytes (0),
	m_bEnabled (TRUE)
{
}

CReassemblyQueue::~CReassemblyQueue (void)
{
	Flush ();

	m_ulMaxBytes = 0;
	m_pRxQueue = nullptr;
}

boolean CReassemblyQueue::Enqueue (u32 nSEG_SEQ, CNetBuffer *pPacket)
{
	if (!m_bEnabled)
	{
		return FALSE;
	}

	assert (pPacket);
	assert (m_ulCurrentBytes <= m_ulMaxBytes);
	size_t ulLength = pPacket->GetLength ();
	if (m_ulCurrentBytes + ulLength > m_ulMaxBytes)
	{
		return FALSE;
	}

	pPacket->SetPrivateData (&nSEG_SEQ, sizeof nSEG_SEQ);

	TPtrListElement *pLastElem = nullptr;
	for (TPtrListElement *pElem = m_List.GetFirst (); pElem; pElem = m_List.GetNext (pElem))
	{
		CNetBuffer *pNetBuffer = static_cast<CNetBuffer *> (CPtrList::GetPtr (pElem));
		assert (pNetBuffer);
		const u32 *pSequenceNumber =
			static_cast<const u32 *> (pNetBuffer->GetPrivateData ());
		assert (pSequenceNumber);
		u32 nSequenceNumber= *pSequenceNumber;

		if (nSEG_SEQ == nSequenceNumber)	// segment already queued
		{
			// disable queue, if length differs from previously received segment
			if (ulLength != pNetBuffer->GetLength ())
			{
				m_bEnabled = FALSE;

				Flush ();

				LOGWARN ("TCP reassembly queue disabled (%u)", 1);
			}

			return FALSE;
		}

		if (lt (nSEG_SEQ, nSequenceNumber))
		{
			// disable queue, if segment overlaps with the following segment
			if (!le (nSEG_SEQ + ulLength, nSequenceNumber))
			{
				m_bEnabled = FALSE;

				Flush ();

				LOGWARN ("TCP reassembly queue disabled (%u)", 2);

				return FALSE;
			}

			m_List.InsertBefore (pElem, pPacket);

			m_ulCurrentBytes += ulLength;

			return TRUE;
		}

		pLastElem = pElem;
	}

	m_List.InsertAfter (pLastElem, pPacket);

	m_ulCurrentBytes += ulLength;

	return TRUE;
}

u32 CReassemblyQueue::Dequeue (u32 nRCV_NXT)
{
	if (!m_bEnabled)
	{
		return nRCV_NXT;
	}

	assert (m_pRxQueue);

	TPtrListElement *pElem;
	while ((pElem = m_List.GetFirst ()) != nullptr)
	{
		CNetBuffer *pNetBuffer = static_cast<CNetBuffer *> (CPtrList::GetPtr (pElem));
		assert (pNetBuffer);
		const u32 *pSequenceNumber =
			static_cast<const u32 *> (pNetBuffer->GetPrivateData ());
		assert (pSequenceNumber);

		if (nRCV_NXT != *pSequenceNumber)
		{
			break;
		}

		size_t ulLength = pNetBuffer->GetLength ();
		nRCV_NXT += ulLength;
		m_ulCurrentBytes -= ulLength;

		m_List.Remove (pElem);

		m_pRxQueue->Enqueue (pNetBuffer);
	}

	return nRCV_NXT;
}

void CReassemblyQueue::Flush (void)
{
	TPtrListElement *pElem;
	while ((pElem = m_List.GetFirst ()) != nullptr)
	{
		CNetBuffer *pNetBuffer = static_cast<CNetBuffer *> (CPtrList::GetPtr (pElem));
		assert (pNetBuffer);

		m_ulCurrentBytes -= pNetBuffer->GetLength ();

		m_List.Remove (pElem);

		delete pNetBuffer;
	}

	assert (!m_ulCurrentBytes);
}
