//
// xhcicommandmanager.cpp
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
#include <circle/usb/xhcicommandmanager.h>
#include <circle/usb/xhcidevice.h>
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <assert.h>

static const char From[] = "xhcicmd";

CXHCICommandManager::CXHCICommandManager (CXHCIDevice *pXHCIDevice)
:	m_pXHCIDevice (pXHCIDevice),
	m_pMMIO (pXHCIDevice->GetMMIOSpace ()),
	m_CmdRing (XHCIRingTypeCommand, XHCI_CONFIG_CMD_RING_SIZE, pXHCIDevice),
	m_bCommandCompleted (TRUE),
	m_pCurrentCommandTRB (0)
{
	if (!m_CmdRing.IsValid ())
	{
		return;
	}

	assert (m_pMMIO != 0);
	m_pMMIO->op_write64 (XHCI_REG_OP_CRCR,   XHCI_TO_DMA (m_CmdRing.GetFirstTRB ())
					       | m_CmdRing.GetCycleState ());
}

CXHCICommandManager::~CXHCICommandManager (void)
{
	assert (m_bCommandCompleted);

	m_pMMIO = 0;
	m_pXHCIDevice = 0;
}

boolean CXHCICommandManager::IsValid (void)
{
	return m_CmdRing.IsValid ();
}

int CXHCICommandManager::EnableSlot (u8 *pSlotID)
{
	u8 uchSlotID;
	int nResult = DoCommand (XHCI_TRB_TYPE_CMD_ENABLE_SLOT << XHCI_TRB_CONTROL_TRB_TYPE__SHIFT,
				 0, 0, 0, &uchSlotID);
	if (!XHCI_CMD_SUCCESS (nResult))
	{
		return nResult;
	}

	if (!XHCI_IS_SLOTID (uchSlotID))
	{
		return XHCI_TRB_COMPLETION_CODE_NO_SLOTS_AVAILABLE_ERROR;
	}

	assert (pSlotID != 0);
	*pSlotID = uchSlotID;

	return nResult;
}

int CXHCICommandManager::DisableSlot (u8 uchSlotID)
{
	assert (XHCI_IS_SLOTID (uchSlotID));

	return DoCommand (  XHCI_TRB_TYPE_CMD_DISABLE_SLOT << XHCI_TRB_CONTROL_TRB_TYPE__SHIFT
			  | (u32) uchSlotID << XHCI_CMD_TRB_DISABLE_SLOT_CONTROL_SLOTID__SHIFT);
}

int CXHCICommandManager::AddressDevice (u8 uchSlotID, TXHCIInputContext *pInputContext,
					boolean bSetAddress)
{
	assert (XHCI_IS_SLOTID (uchSlotID));
	u32 nControl =   XHCI_TRB_TYPE_CMD_ADDRESS_DEVICE << XHCI_TRB_CONTROL_TRB_TYPE__SHIFT
		       | (u32) uchSlotID << XHCI_CMD_TRB_ADDRESS_DEVICE_CONTROL_SLOTID__SHIFT;
	if (!bSetAddress)
	{
		nControl |= XHCI_CMD_TRB_ADDRESS_DEVICE_CONTROL_BSR;
	}

	assert (pInputContext != 0);
	return DoCommand (nControl, XHCI_TO_DMA_LO (pInputContext), XHCI_TO_DMA_HI (pInputContext));
}

int CXHCICommandManager::ConfigureEndpoint (u8 uchSlotID, TXHCIInputContext *pInputContext,
					    boolean bDeconfigure)
{
	assert (XHCI_IS_SLOTID (uchSlotID));
	u32 nControl =   XHCI_TRB_TYPE_CMD_CONFIGURE_ENDPOINT << XHCI_TRB_CONTROL_TRB_TYPE__SHIFT
		       | (u32) uchSlotID << XHCI_CMD_TRB_CONFIGURE_ENDPOINT_CONTROL_SLOTID__SHIFT;
	if (bDeconfigure)
	{
		nControl |= XHCI_CMD_TRB_CONFIGURE_ENDPOINT_CONTROL_DC;
	}

	assert (pInputContext != 0);
	return DoCommand (nControl, XHCI_TO_DMA_LO (pInputContext), XHCI_TO_DMA_HI (pInputContext));
}

int CXHCICommandManager::EvaluateContext (u8 uchSlotID, TXHCIInputContext *pInputContext)
{
	assert (XHCI_IS_SLOTID (uchSlotID));
	u32 nControl =   XHCI_TRB_TYPE_CMD_EVALUATE_CONTEXT << XHCI_TRB_CONTROL_TRB_TYPE__SHIFT
		       | (u32) uchSlotID << XHCI_CMD_TRB_EVALUATE_CONTEXT_CONTROL_SLOTID__SHIFT;

	assert (pInputContext != 0);
	return DoCommand (nControl, XHCI_TO_DMA_LO (pInputContext), XHCI_TO_DMA_HI (pInputContext));
}

int CXHCICommandManager::NoOp (void)
{
	return DoCommand (XHCI_TRB_TYPE_CMD_NO_OP << XHCI_TRB_CONTROL_TRB_TYPE__SHIFT);
}

#ifndef NDEBUG

void CXHCICommandManager::DumpStatus (void)
{
	if (!m_bCommandCompleted)
	{
		CLogger::Get ()->Write (From, LogDebug, "Command is running (control 0x%X)",
					m_pCurrentCommandTRB->Control);
	}

	m_CmdRing.DumpStatus (From);
}

#endif

int CXHCICommandManager::DoCommand (u32 nControl, u32 nParameter1, u32 nParameter2, u32 nStatus,
				    u8 *pSlotID)
{
#ifdef XHCI_DEBUG
	CLogger::Get ()->Write (From, LogDebug, "Execute command %u (control 0x%X)",
				   (nControl & XHCI_TRB_CONTROL_TRB_TYPE__MASK)
				>> XHCI_TRB_CONTROL_TRB_TYPE__SHIFT,
				nControl);
#endif

	assert (m_bCommandCompleted);

	TXHCITRB *pCmdTRB = m_CmdRing.GetEnqueueTRB ();
	if (pCmdTRB == 0)
	{
		return XHCI_TRB_COMPLETION_CODE_RING_OVERRUN;
	}

	pCmdTRB->Parameter1 = nParameter1;
	pCmdTRB->Parameter2 = nParameter2;
	pCmdTRB->Status = nStatus;

	assert (!(nControl & XHCI_TRB_CONTROL_C));
	pCmdTRB->Control = nControl | m_CmdRing.GetCycleState ();

	m_pCurrentCommandTRB = pCmdTRB;
	m_bCommandCompleted = FALSE;

	m_CmdRing.IncrementEnqueue ();

	DataSyncBarrier ();

	m_pMMIO->db_write32 (XHCI_REG_DB_HOST_CONTROLLER, XHCI_REG_DB_TARGET_HC_COMMAND);

	unsigned nStartTicks = CTimer::Get ()->GetTicks ();
	while (!m_bCommandCompleted)
	{
		if (CTimer::Get ()->GetTicks () - nStartTicks >= 3*HZ)
		{
#ifdef XHCI_DEBUG
			CLogger::Get ()->Write (From, LogDebug,
						"Command timed out (control 0x%X)", nControl);
#endif

			m_bCommandCompleted = TRUE;
			m_pCurrentCommandTRB = 0;

			return XHCI_PRIV_COMPLETION_CODE_TIMEOUT;
		}
	}

	DataMemBarrier ();

	if (pSlotID != 0)
	{
		*pSlotID = m_uchSlotID;
	}

	return m_uchCompletionCode;
}

void CXHCICommandManager::CommandCompleted (TXHCITRB *pCommandTRB, u8 uchCompletionCode,
					    u8 uchSlotID)
{
	assert (pCommandTRB != 0);

#ifdef XHCI_DEBUG
	u8 uchCommandType =    (pCommandTRB->Control & XHCI_TRB_CONTROL_TRB_TYPE__MASK)
			    >> XHCI_TRB_CONTROL_TRB_TYPE__SHIFT;

	CLogger::Get ()->Write (From, LogDebug, "Command %u completed with %u (slot %u)",
				(unsigned) uchCommandType, (unsigned) uchCompletionCode,
				(unsigned) uchSlotID);
#endif

	if (   m_bCommandCompleted			// does happen on timeout only
	    || m_pCurrentCommandTRB != pCommandTRB)
	{
		return;
	}

	m_pCurrentCommandTRB = 0;

	m_uchCompletionCode = uchCompletionCode;
	m_uchSlotID = uchSlotID;

	DataMemBarrier ();

	m_bCommandCompleted = TRUE;
}
