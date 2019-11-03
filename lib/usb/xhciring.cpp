//
// xhciring.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/xhciring.h>
#include <circle/usb/xhcidevice.h>
#include <circle/logger.h>
#include <circle/debug.h>
#include <assert.h>

static const char From[] = "xhciring";

CXHCIRing::CXHCIRing (TXHCIRingType Type, unsigned nTRBCount, CXHCIDevice *pAllocator)
:	m_Type (Type),
	m_nTRBCount (nTRBCount),
	m_pAllocator (pAllocator),
	m_pFirstTRB (0),
	m_nEnqueueIndex (0),
	m_nDequeueIndex (0),
	m_nCycleState (XHCI_TRB_CONTROL_C)
{
	assert (m_nTRBCount >= 16);
	assert (m_nTRBCount % 4 == 0);

	assert (m_pAllocator != 0);
	m_pFirstTRB = (TXHCITRB *) m_pAllocator->AllocateSharedMem (m_nTRBCount * sizeof (TXHCITRB),
								    64, 0x10000);
	if (m_pFirstTRB == 0)
	{
		return;
	}

	if (m_Type != XHCIRingTypeEvent)
	{
		TXHCITRB *pLinkTRB = &m_pFirstTRB[m_nTRBCount - 1];

		pLinkTRB->Parameter = XHCI_TO_DMA (m_pFirstTRB);
		pLinkTRB->Status = 0;
		pLinkTRB->Control =   XHCI_TRB_TYPE_LINK << XHCI_TRB_CONTROL_TRB_TYPE__SHIFT
				    | XHCI_LINK_TRB_CONTROL_TC;
	}
}

CXHCIRing::~CXHCIRing (void)
{
	if (m_pFirstTRB != 0)
	{
		m_pAllocator->FreeSharedMem (m_pFirstTRB);

		m_pFirstTRB = 0;
	}
}

boolean CXHCIRing::IsValid (void) const
{
	return m_pFirstTRB != 0;
}

unsigned CXHCIRing::GetTRBCount (void) const
{
	assert (m_pFirstTRB != 0);

	return m_nTRBCount;
}

TXHCITRB *CXHCIRing::GetFirstTRB (void)
{
	assert (m_pFirstTRB != 0);

	return m_pFirstTRB;
}

TXHCITRB *CXHCIRing::GetDequeueTRB (void)
{
	assert (m_pFirstTRB != 0);
	assert (m_nDequeueIndex < m_nTRBCount);

	if ((m_pFirstTRB[m_nDequeueIndex].Control & XHCI_TRB_CONTROL_C) != m_nCycleState)
	{
		return 0;		// ring is empty
	}

	return &m_pFirstTRB[m_nDequeueIndex];
}

TXHCITRB *CXHCIRing::GetEnqueueTRB (void)
{
	assert (m_pFirstTRB != 0);
	assert (m_nEnqueueIndex < m_nTRBCount);

	if ((m_pFirstTRB[m_nEnqueueIndex].Control & XHCI_TRB_CONTROL_C) == m_nCycleState)
	{
		return 0;		// ring is full
	}

	return &m_pFirstTRB[m_nEnqueueIndex];
}

TXHCITRB *CXHCIRing::IncrementDequeue (void)
{
	assert (m_pFirstTRB != 0);
	assert (m_Type == XHCIRingTypeEvent);
	assert (m_nDequeueIndex < m_nTRBCount);

	assert (   (m_pFirstTRB[m_nDequeueIndex].Control & XHCI_TRB_CONTROL_C)
		== m_nCycleState);	// there must be an entry on ring

	if (++m_nDequeueIndex == m_nTRBCount)
	{
		m_nDequeueIndex = 0;

		m_nCycleState ^= XHCI_TRB_CONTROL_C;
	}

	return &m_pFirstTRB[m_nDequeueIndex];
}

void CXHCIRing::IncrementEnqueue (void)
{
	assert (m_pFirstTRB != 0);
	assert (m_Type != XHCIRingTypeEvent);
	assert (m_nEnqueueIndex < m_nTRBCount);

	assert (   (m_pFirstTRB[m_nEnqueueIndex].Control & XHCI_TRB_CONTROL_C)
		== m_nCycleState);	// Cycle state must be already set

	if (++m_nEnqueueIndex == m_nTRBCount-1)		// last index is used for Link TRB
	{
		TXHCITRB *pLinkTRB = &m_pFirstTRB[m_nEnqueueIndex];

		pLinkTRB->Control ^= XHCI_TRB_CONTROL_C;

		if (pLinkTRB->Control & XHCI_LINK_TRB_CONTROL_TC)
		{
			m_nCycleState ^= XHCI_TRB_CONTROL_C;
		}

		m_nEnqueueIndex = 0;
	}
}

u32 CXHCIRing::GetCycleState (void) const
{
	assert (m_pFirstTRB != 0);

	return m_nCycleState;
}

#ifndef NDEBUG

void CXHCIRing::DumpStatus (const char *pFrom)
{
	CLogger::Get ()->Write (pFrom != 0 ? pFrom : From, LogDebug, "Count %u, %s %u, Cycle %u",
				m_nTRBCount,
				m_Type == XHCIRingTypeEvent ? "Dequeue" : "Enqueue",
				m_Type == XHCIRingTypeEvent ? m_nDequeueIndex : m_nEnqueueIndex,
				m_nCycleState);

	if (m_pFirstTRB != 0)
	{
		debug_hexdump (m_pFirstTRB, m_nTRBCount * sizeof (TXHCITRB),
			       pFrom != 0 ? pFrom : From);
	}
}

#endif
