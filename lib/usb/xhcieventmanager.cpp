//
// xhcieventmanager.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019-2022  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/xhcieventmanager.h>
#include <circle/usb/xhcidevice.h>
#include <circle/logger.h>
#include <assert.h>

static const char From[] = "xhcievt";

CXHCIEventManager::CXHCIEventManager (CXHCIDevice *pXHCIDevice)
:	m_pXHCIDevice (pXHCIDevice),
	m_pMMIO (pXHCIDevice->GetMMIOSpace ()),
	m_EventRing (XHCIRingTypeEvent, XHCI_CONFIG_EVENT_RING_SIZE, pXHCIDevice),
	m_pERST (0)
{
	if (!m_EventRing.IsValid ())
	{
		return;
	}

	assert (m_pXHCIDevice != 0);
	m_pERST = (TXHCIERSTEntry *) m_pXHCIDevice->AllocateSharedMem (sizeof (TXHCIERSTEntry));
	if (m_pERST == 0)
	{
		return;
	}

	m_pERST->RingSegmentBase = XHCI_TO_DMA (m_EventRing.GetFirstTRB ());
	m_pERST->RingSegmentSize = m_EventRing.GetTRBCount ();
	m_pERST->Reserved = 0;

	assert (m_pMMIO != 0);
	m_pMMIO->rt_write32 (0, XHCI_REG_RT_IR_ERSTSZ, 1);
	m_pMMIO->rt_write64 (0, XHCI_REG_RT_IR_ERSTBA_LO, XHCI_TO_DMA (m_pERST));
	m_pMMIO->rt_write64 (0, XHCI_REG_RT_IR_ERDP_LO, XHCI_TO_DMA (m_EventRing.GetFirstTRB ()));
	m_pMMIO->rt_write32 (0, XHCI_REG_RT_IR_IMOD, XHCI_CONFIG_IMODI);
	m_pMMIO->rt_write32 (0, XHCI_REG_RT_IR_IMAN,   m_pMMIO->rt_read32 (0, XHCI_REG_RT_IR_IMAN)
						     | XHCI_REG_RT_IR_IMAN_IE);
}

CXHCIEventManager::~CXHCIEventManager (void)
{
	if (m_pERST != 0)
	{
		assert (m_pMMIO != 0);
		m_pMMIO->rt_write32 (0, XHCI_REG_RT_IR_IMAN,
				       m_pMMIO->rt_read32 (0, XHCI_REG_RT_IR_IMAN)
				     & ~XHCI_REG_RT_IR_IMAN_IE);

		assert (m_pXHCIDevice != 0);
		m_pXHCIDevice->FreeSharedMem (m_pERST);

		m_pERST = 0;
	}

	m_pMMIO = 0;
	m_pXHCIDevice = 0;
}

boolean CXHCIEventManager::IsValid (void)
{
	return m_EventRing.IsValid () && m_pERST != 0;
}

TXHCITRB *CXHCIEventManager::HandleEvents (void)
{
	assert (m_pXHCIDevice != 0);

	TXHCITRB *pEventTRB = m_EventRing.GetDequeueTRB ();
	if (pEventTRB == 0)
	{
		return 0;
	}

	unsigned nTRBType =    (pEventTRB->Control & XHCI_TRB_CONTROL_TRB_TYPE__MASK)
			    >> XHCI_TRB_CONTROL_TRB_TYPE__SHIFT;
	switch (nTRBType)
	{
	case XHCI_TRB_TYPE_EVENT_TRANSFER:
		m_pXHCIDevice->GetSlotManager ()->TransferEvent (
			pEventTRB->Status >> XHCI_EVENT_TRB_STATUS_COMPLETION_CODE__SHIFT,
			pEventTRB->Status & XHCI_TRANSFER_EVENT_TRB_STATUS_TRB_TRANSFER_LENGTH__MASK,
			pEventTRB->Control >> XHCI_CMD_COMPLETION_EVENT_TRB_CONTROL_SLOTID__SHIFT,
			   (pEventTRB->Control & XHCI_TRANSFER_EVENT_TRB_CONTROL_ENDPOINTID__MASK)
			>> XHCI_TRANSFER_EVENT_TRB_CONTROL_ENDPOINTID__SHIFT);
		break;

	case XHCI_TRB_TYPE_EVENT_CMD_COMPLETION:
		m_pXHCIDevice->GetCommandManager ()->CommandCompleted (
			(TXHCITRB *) XHCI_FROM_DMA (pEventTRB->Parameter),
			pEventTRB->Status >> XHCI_EVENT_TRB_STATUS_COMPLETION_CODE__SHIFT,
			pEventTRB->Control >> XHCI_CMD_COMPLETION_EVENT_TRB_CONTROL_SLOTID__SHIFT);
		break;

	case XHCI_TRB_TYPE_EVENT_PORT_STATUS_CHANGE:
		assert (   pEventTRB->Status >> XHCI_EVENT_TRB_STATUS_COMPLETION_CODE__SHIFT
			== XHCI_TRB_COMPLETION_CODE_SUCCESS);
		m_pXHCIDevice->GetRootHub ()->StatusChanged (pEventTRB->Parameter1
			>> XHCI_PORT_STATUS_EVENT_TRB_PARAMETER1_PORTID__SHIFT);
		break;

	case XHCI_TRB_TYPE_EVENT_HOST_CONTROLLER: {
		u8 uchCompletionCode =    pEventTRB->Status
				       >> XHCI_EVENT_TRB_STATUS_COMPLETION_CODE__SHIFT;
		if (uchCompletionCode == XHCI_TRB_COMPLETION_CODE_EVENT_RING_FULL_ERROR)
		{
			CLogger::Get ()->Write (From, LogPanic, "Event ring full");
		}
#ifndef NDEBUG
		m_pXHCIDevice->DumpStatus ();
#endif
		CLogger::Get ()->Write (From, LogPanic, "HC event (completion %u)",
					(unsigned) uchCompletionCode);
		} break;

	default:
		CLogger::Get ()->Write (From, LogPanic, "Unhandled TRB (type %u)", nTRBType);
		break;
	}

	pEventTRB = m_EventRing.IncrementDequeue ();
	assert (pEventTRB != 0);

	return pEventTRB;
}

#ifndef NDEBUG

void CXHCIEventManager::DumpStatus (void)
{
	m_EventRing.DumpStatus (From);
}

#endif
