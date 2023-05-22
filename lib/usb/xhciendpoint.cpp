//
// xhciendpoint.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019-2023  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/xhciendpoint.h>
#include <circle/usb/xhcidevice.h>
#include <circle/usb/xhciusbdevice.h>
#include <circle/usb/xhcicommandmanager.h>
#include <circle/usb/usbrequest.h>
#include <circle/sched/scheduler.h>
#include <circle/synchronize.h>
#include <circle/sysconfig.h>
#include <circle/logger.h>
#include <circle/timer.h>
#include <circle/util.h>
#include <circle/debug.h>
#include <assert.h>

static const char From[] = "xhciep";

CXHCIEndpoint::CXHCIEndpoint (CXHCIUSBDevice *pDevice, CXHCIDevice *pXHCIDevice)
:	m_pDevice (pDevice),
	m_pXHCIDevice (pXHCIDevice),
	m_pMMIO (pXHCIDevice->GetMMIOSpace ()),
	m_bValid (TRUE),
	m_pTransferRing (0),
	m_uchEndpointID (1),
	m_uchEndpointType (XHCI_EP_CONTEXT_EP_TYPE_CONTROL),
	m_pURB {0, 0},
	m_bTransferCompleted (TRUE),
	m_pInputContextBuffer (0)
{
	m_pTransferRing = new CXHCIRing (XHCIRingTypeTransfer,
					 XHCI_CONFIG_TRANSFER_RING_SIZE, pXHCIDevice);
	if (   m_pTransferRing == 0
	    || !m_pTransferRing->IsValid ())
	{
		m_bValid = FALSE;

		return;
	}

	m_pDevice->RegisterEndpoint (m_uchEndpointID, this);
}

CXHCIEndpoint::CXHCIEndpoint (CXHCIUSBDevice *pDevice, const TUSBEndpointDescriptor *pDesc,
			      CXHCIDevice *pXHCIDevice)
:	m_pDevice (pDevice),
	m_pXHCIDevice (pXHCIDevice),
	m_pMMIO (pXHCIDevice->GetMMIOSpace ()),
	m_bValid (TRUE),
	m_pTransferRing (0),
	m_uchEndpointID (0),
	m_uchEndpointType (0),
	m_pURB {0, 0},
	m_bTransferCompleted (TRUE),
	m_pInputContextBuffer (0)
{
	m_pTransferRing = new CXHCIRing (XHCIRingTypeTransfer,
					 XHCI_CONFIG_TRANSFER_RING_SIZE, pXHCIDevice);
	if (   m_pTransferRing == 0
	    || !m_pTransferRing->IsValid ())
	{
		m_bValid = FALSE;

		return;
	}

	// copy endpoint descriptor
	assert (pDesc != 0);
	assert (pDesc->bLength >= sizeof *pDesc);	// may have class-specific trailer
	assert (pDesc->bDescriptorType == DESCRIPTOR_ENDPOINT);

	m_uchEndpointAddress = pDesc->bEndpointAddress;
	m_uchAttributes = pDesc->bmAttributes;
	m_usMaxPacketSize = pDesc->wMaxPacketSize & 0x7FF;

	assert (m_pDevice != 0);
	if ((m_uchAttributes & 1) == 1)		// interrupt or isochronous endpoint
	{
		m_uchInterval = ConvertInterval (pDesc->bInterval, m_pDevice->GetSpeed ());
	}
	else
	{
		m_uchInterval = 0;
	}

	// workaround for low-speed devices with bulk endpoints,
	// which is normally forbidden by the USB spec.
	if (   m_pDevice->GetSpeed () == USBSpeedLow
	    && (m_uchAttributes & 3) == 2)
	{
		m_uchAttributes = 3;		// fake interrupt endpoint

		if (m_usMaxPacketSize > 8)
		{
			m_usMaxPacketSize = 8;
		}

		m_uchInterval = ConvertInterval (1, USBSpeedLow);
	}

	// calculate endpoint ID
	assert ((m_uchEndpointAddress & 0x0F) >= 1);
	m_uchEndpointID = (m_uchEndpointAddress & 0x0F) * 2;
	if (m_uchEndpointAddress & 0x80)
	{
		m_uchEndpointID++;
	}
	assert (XHCI_IS_ENDPOINTID (m_uchEndpointID));

	// calculate endpoint type ID
	m_uchEndpointType = m_uchAttributes & 3;
	if (m_uchEndpointType == 0 || m_uchEndpointAddress & 0x80)
	{
		m_uchEndpointType += 4;
	}

	// configure endpoint on HC
	TXHCIInputContext *pInputContext = GetInputContextConfigureEndpoint ();
	assert (pInputContext != 0);

	assert (m_pXHCIDevice != 0);
	int nResult = m_pXHCIDevice->GetCommandManager ()->ConfigureEndpoint (
			m_pDevice->GetSlotID (), pInputContext, FALSE);

	FreeInputContext ();

	if (!XHCI_CMD_SUCCESS (nResult))
	{
		m_bValid = FALSE;

		return;
	}

	m_pDevice->RegisterEndpoint (m_uchEndpointID, this);
}

CXHCIEndpoint::~CXHCIEndpoint (void)
{
	assert (m_bTransferCompleted);
	assert (m_pInputContextBuffer == 0);

	m_uchEndpointID = 0;

	delete m_pTransferRing;
	m_pTransferRing = 0;

	m_bValid = FALSE;

	m_pDevice = 0;
	m_pXHCIDevice = 0;
	m_pMMIO = 0;
}

boolean CXHCIEndpoint::IsValid (void)
{
	return m_bValid;
}

CXHCIRing *CXHCIEndpoint::GetTransferRing (void)
{
	assert (m_pTransferRing != 0);
	return m_pTransferRing;
}

boolean CXHCIEndpoint::SetMaxPacketSize (u32 nMaxPacketSize)
{
	assert (m_uchEndpointID == 1);			// only called for EP0
	if (m_pDevice->GetSpeed () != USBSpeedFull)	// already set if not full speed device
	{
		return TRUE;
	}

	assert (8 <= nMaxPacketSize && nMaxPacketSize <= 64);
	m_usMaxPacketSize = nMaxPacketSize;

	TXHCIInputContext *pInputContext = GetInputContextSetMaxPacketSize ();
	assert (pInputContext != 0);

	assert (m_pXHCIDevice != 0);
	assert (m_pDevice != 0);
	int nResult = m_pXHCIDevice->GetCommandManager ()->EvaluateContext (m_pDevice->GetSlotID (),
									    pInputContext);

	FreeInputContext ();

	return XHCI_CMD_SUCCESS (nResult);
}

boolean CXHCIEndpoint::Transfer (CUSBRequest *pURB, unsigned nTimeoutMs)
{
	assert (pURB != 0);
	assert (nTimeoutMs == USB_TIMEOUT_NONE);	// TODO

	pURB->SetCompletionRoutine (CompletionRoutine, 0, this);

	assert (m_bTransferCompleted);
	m_bTransferCompleted = FALSE;

	if (!TransferAsync (pURB, nTimeoutMs))
	{
		return FALSE;
	}

	unsigned nStartTicks = CTimer::Get ()->GetTicks ();
	while (!m_bTransferCompleted)
	{
		if (CTimer::Get ()->GetTicks () - nStartTicks >= HZ)
		{
			CLogger::Get ()->Write (From, LogDebug, "Transfer timed out");
#ifdef XHCI_DEBUG
			m_pDevice->DumpStatus ();
#endif

			m_SpinLock.Acquire ();
			m_pURB[0] = m_pURB[1];
			m_pURB[1] = 0;
			m_SpinLock.Release ();
			m_bTransferCompleted = TRUE;

			return FALSE;
		}

#ifdef NO_BUSY_WAIT
		CScheduler::Get ()->Yield ();
#endif
	}

	DataMemBarrier ();

	return pURB->GetStatus () ? TRUE : FALSE;
}

boolean CXHCIEndpoint::TransferAsync (CUSBRequest *pURB, unsigned nTimeoutMs)
{
	assert (pURB != 0);
	assert (nTimeoutMs == USB_TIMEOUT_NONE);	// TODO

	if (!m_bValid)
	{
		return FALSE;
	}

	void *pBuffer = pURB->GetBuffer ();
	u32 nBufLen = pURB->GetBufLen ();

	m_SpinLock.Acquire ();
	if (m_pURB[0] == 0)
	{
		m_pURB[0] = pURB;
	}
	else
	{
		assert (m_pURB[1] == 0);
		m_pURB[1] = pURB;
	}
	m_SpinLock.Release ();

	if (   (m_uchEndpointType & 3) == 2		// bulk EP
	    || (m_uchEndpointType & 3) == 3)		// interrupt EP
	{

		assert (pBuffer != 0);
		assert (nBufLen > 0);
		assert ((uintptr) pBuffer > MEM_KERNEL_END);
		CleanAndInvalidateDataCacheRange ((uintptr) pBuffer, nBufLen);

		if (!EnqueueTRB (  XHCI_TRB_TYPE_NORMAL << XHCI_TRB_CONTROL_TRB_TYPE__SHIFT
				 | XHCI_TRANSFER_TRB_CONTROL_IOC,
		     		 nBufLen | 0 << XHCI_TRANSFER_TRB_STATUS_TD_SIZE__SHIFT,
				 XHCI_TO_DMA_LO (pBuffer),
				 XHCI_TO_DMA_HI (pBuffer)))
		{
			goto EnqueueError;
		}
	}
	else if (m_uchEndpointType == 4)		// control EP
	{
		TSetupData *pSetup = pURB->GetSetupData ();
		assert (pSetup != 0);

		u8 uchTRT;			// transfer type
		u32 nDirData = 0;		// direction flags
		u32 nDirStatus = 0;
		if (pSetup->bmRequestType & REQUEST_IN)
		{
			assert (nBufLen > 0);

			uchTRT = XHCI_TRANSFER_TRB_CONTROL_TRT_IN;
			nDirData = XHCI_TRANSFER_TRB_CONTROL_DIR_IN;
		}
		else
		{
			if (nBufLen == 0)
			{
				uchTRT = XHCI_TRANSFER_TRB_CONTROL_TRT_NODATA;
			}
			else
			{
				uchTRT = XHCI_TRANSFER_TRB_CONTROL_TRT_OUT;
			}

			nDirStatus = XHCI_TRANSFER_TRB_CONTROL_DIR_IN;
		}

		// SETUP stage
		if (!EnqueueTRB (  XHCI_TRB_TYPE_SETUP_STAGE << XHCI_TRB_CONTROL_TRB_TYPE__SHIFT
				 | uchTRT << XHCI_TRANSFER_TRB_CONTROL_TRT__SHIFT
				 | XHCI_TRANSFER_TRB_CONTROL_IDT,
				 sizeof (TSetupData),
				   pSetup->bmRequestType | (u32) pSetup->bRequest << 8
				 | (u32) pSetup->wValue << 16,
				 pSetup->wIndex | (u32) pSetup->wLength << 16))
		{
			goto EnqueueError;
		}

		// DATA stage
		if (uchTRT != XHCI_TRANSFER_TRB_CONTROL_TRT_NODATA)
		{
			assert (pBuffer != 0);
			assert (nBufLen > 0);
			assert ((uintptr) pBuffer > MEM_KERNEL_END);
			CleanAndInvalidateDataCacheRange ((uintptr) pBuffer, nBufLen);

			if (!EnqueueTRB (  XHCI_TRB_TYPE_DATA_STAGE << XHCI_TRB_CONTROL_TRB_TYPE__SHIFT
					 | nDirData,
					 nBufLen | 0 << XHCI_TRANSFER_TRB_STATUS_TD_SIZE__SHIFT,
					 XHCI_TO_DMA_LO (pBuffer),
					 XHCI_TO_DMA_HI (pBuffer)))
			{
				goto EnqueueError;
			}
		}

		// STATUS stage
		if (!EnqueueTRB (  XHCI_TRB_TYPE_STATUS_STAGE << XHCI_TRB_CONTROL_TRB_TYPE__SHIFT
				 | nDirStatus | XHCI_TRANSFER_TRB_CONTROL_IOC))
		{
			goto EnqueueError;
		}
	}
	else
	{
		assert ((m_uchEndpointType & 3) == 1);	// isochronous EP

		assert (pBuffer != 0);
		assert (nBufLen > 0);
		assert ((uintptr) pBuffer > MEM_KERNEL_END);
		CleanAndInvalidateDataCacheRange ((uintptr) pBuffer, nBufLen);

		u32 nPackets = pURB->GetNumIsoPackets ();
		for (unsigned i = 0; i < nPackets; i++)
		{
			u16 usPacketSize = pURB->GetIsoPacketSize (i);

			if (!EnqueueTRB (  XHCI_TRB_TYPE_ISOCH << XHCI_TRB_CONTROL_TRB_TYPE__SHIFT
					 | (i == nPackets-1 ? XHCI_TRANSFER_TRB_CONTROL_IOC : 0)
					 | XHCI_TRANSFER_TRB_CONTROL_SIA,
					   usPacketSize
					 | (nPackets-i-1) << XHCI_TRANSFER_TRB_STATUS_TD_SIZE__SHIFT,
					 XHCI_TO_DMA_LO (pBuffer),
					 XHCI_TO_DMA_HI (pBuffer)))
			{
				goto EnqueueError;
			}

			pBuffer = (u8 *) pBuffer + usPacketSize;
		}
	}

	DataSyncBarrier ();

	assert (m_pDevice != 0);
	assert (XHCI_IS_ENDPOINTID (m_uchEndpointID));
	m_pMMIO->db_write32 (m_pDevice->GetSlotID (), XHCI_REG_DB_TARGET_EP0 + m_uchEndpointID-1);

	return TRUE;

EnqueueError:
	m_SpinLock.Acquire ();
	if (m_pURB[1] != 0)
	{
		m_pURB[1] = 0;
	}
	else
	{
		m_pURB[0] = 0;
	}
	m_SpinLock.Release ();

	return FALSE;
}

void CXHCIEndpoint::TransferEvent (u8 uchCompletionCode, u32 nTransferLength)
{
#ifdef XHCI_DEBUG2
	CLogger::Get ()->Write (From, LogDebug,
				"Transfer event on endpoint %u (completion %u, length %u)",
			        (unsigned) m_uchEndpointID, (unsigned) uchCompletionCode,
				nTransferLength);
#endif

	DataMemBarrier ();

	CUSBRequest *pURB = m_pURB[0];
	if (!pURB)
	{
		return;
	}

	if (   XHCI_TRB_SUCCESS (uchCompletionCode)
	    || uchCompletionCode == XHCI_TRB_COMPLETION_CODE_SHORT_PACKET)
	{
		void *pBuffer = pURB->GetBuffer ();
		u32 nBufLen = pURB->GetBufLen ();
		if (pBuffer != 0)
		{
			assert (nBufLen > 0);
			CleanAndInvalidateDataCacheRange ((uintptr) pBuffer, nBufLen);
		}

		assert (nTransferLength <= nBufLen);
		pURB->SetResultLen (nBufLen - nTransferLength);

		pURB->SetStatus (1);
	}
	else if (   uchCompletionCode == XHCI_TRB_COMPLETION_CODE_RING_UNDERRUN
		 || uchCompletionCode == XHCI_TRB_COMPLETION_CODE_RING_OVERRUN)
	{
		// these events are not URB related, so just ignore them
		return;
	}
	else if (pURB->GetEndpoint ()->GetType () != EndpointTypeIsochronous)
	{
		CLogger::Get ()->Write (From, LogWarning, "Transfer error %u on endpoint %u",
					(unsigned) uchCompletionCode, (unsigned) m_uchEndpointID);
	}

	m_SpinLock.Acquire ();
	m_pURB[0] = m_pURB[1];
	m_pURB[1] = 0;
	m_SpinLock.Release ();

	pURB->CallCompletionRoutine ();
}

#ifndef NDEBUG

void CXHCIEndpoint::DumpStatus (void)
{
	if (!m_bValid)
	{
		CLogger::Get ()->Write (From, LogDebug, "Endpoint not valid");

		return;
	}

	if (m_uchEndpointID > 1)
	{
		CLogger::Get ()->Write (From, LogDebug,
					"Endpoint %u (addr 0x%X, attr 0x%X, maxpkt %u, interval %u)",
					(unsigned) m_uchEndpointID,
					(unsigned) m_uchEndpointAddress,
					(unsigned) m_uchAttributes,
					(unsigned) m_usMaxPacketSize,
					(unsigned) m_uchInterval);
	}

	assert (m_pTransferRing != 0);
	m_pTransferRing->DumpStatus (From);
}

#endif

void CXHCIEndpoint::CompletionRoutine (CUSBRequest *pURB, void *pParam, void *pContext)
{
	CXHCIEndpoint *pThis = (CXHCIEndpoint *) pContext;
	assert (pThis != 0);

	assert (!pThis->m_bTransferCompleted);
	pThis->m_bTransferCompleted = TRUE;
}

boolean CXHCIEndpoint::EnqueueTRB (u32 nControl, u32 nStatus, u32 nParameter1, u32 nParameter2)
{
	assert (m_pTransferRing != 0);
	TXHCITRB *pTransferTRB = m_pTransferRing->GetEnqueueTRB ();
	if (pTransferTRB == 0)
	{
		return 0;
	}

	pTransferTRB->Parameter1 = nParameter1;
	pTransferTRB->Parameter2 = nParameter2;

	pTransferTRB->Status =   nStatus
			       |    XHCI_INTERRUPTER_TARGET_DEFAULT
			         << XHCI_TRANSFER_TRB_STATUS_INTERRUPTER_TARGET__SHIFT;

	pTransferTRB->Control = nControl | m_pTransferRing->GetCycleState ();

	m_pTransferRing->IncrementEnqueue ();

	return pTransferTRB;
}

TXHCIInputContext *CXHCIEndpoint::GetInputContextSetMaxPacketSize (void)
{
	assert (m_pInputContextBuffer == 0);
	m_pInputContextBuffer = new u8[sizeof (TXHCIInputContext) + XHCI_PAGE_SIZE-1];
	assert (m_pInputContextBuffer != 0);

	TXHCIInputContext *pInputContext =
		(TXHCIInputContext *) XHCI_ALIGN_PTR (m_pInputContextBuffer, XHCI_PAGE_SIZE);

	assert (m_pDevice != 0);
	memset (&pInputContext->Control, 0, sizeof pInputContext->Control);
	memcpy (&pInputContext->Device, m_pDevice->GetDeviceContext (),
		sizeof pInputContext->Device);

	// set input control context
	pInputContext->Control.AddContextFlags = 2;		// set A1
	pInputContext->Control.DropContextFlags = 2;		// set D1

	// set EP context
	TXHCIEndpointContext *pEPContext = &pInputContext->Device.Endpoint[0];

	pEPContext->MaxPacketSize = m_usMaxPacketSize;

	CleanAndInvalidateDataCacheRange ((uintptr) pInputContext, sizeof *pInputContext);

	return pInputContext;
}

TXHCIInputContext *CXHCIEndpoint::GetInputContextConfigureEndpoint (void)
{
	assert (m_pInputContextBuffer == 0);
	m_pInputContextBuffer = new u8[sizeof (TXHCIInputContext) + XHCI_PAGE_SIZE-1];
	assert (m_pInputContextBuffer != 0);

	TXHCIInputContext *pInputContext =
		(TXHCIInputContext *) XHCI_ALIGN_PTR (m_pInputContextBuffer, XHCI_PAGE_SIZE);

	memset (pInputContext, 0, sizeof *pInputContext);

	// set input control context
	assert (XHCI_IS_ENDPOINTID (m_uchEndpointID));
	pInputContext->Control.AddContextFlags = 1 << m_uchEndpointID;
	pInputContext->Control.DropContextFlags = 1 << m_uchEndpointID;

	// set EP context
	TXHCIEndpointContext *pEPContext = &pInputContext->Device.Endpoint[m_uchEndpointID-1];

	assert (m_pTransferRing != 0);
	pEPContext->TRDequeuePointer =
		XHCI_TO_DMA (m_pTransferRing->GetFirstTRB ()) | XHCI_EP_CONTEXT_TR_DEQUEUE_PTR_DCS;

	pEPContext->EPType = m_uchEndpointType;
	pEPContext->MaxPacketSize = m_usMaxPacketSize;
	pEPContext->MaxBurstSize = 0;		// TODO
	pEPContext->MaxPStreams = 0;
	pEPContext->CErr = 3;

	switch (m_uchEndpointType)
	{
	case XHCI_EP_CONTEXT_EP_TYPE_BULK_OUT:
	case XHCI_EP_CONTEXT_EP_TYPE_BULK_IN:
		pEPContext->AverageTRBLength = 256;	// best guess
		break;

	case XHCI_EP_CONTEXT_EP_TYPE_INTERRUPT_OUT:
	case XHCI_EP_CONTEXT_EP_TYPE_INTERRUPT_IN:
		pEPContext->Interval = m_uchInterval;
		pEPContext->AverageTRBLength = 16;	// best guess
		pEPContext->MaxESITPayload = m_usMaxPacketSize;
		break;

	case XHCI_EP_CONTEXT_EP_TYPE_ISOCH_OUT:
	case XHCI_EP_CONTEXT_EP_TYPE_ISOCH_IN:
		pEPContext->Interval = m_uchInterval;
		pEPContext->AverageTRBLength = m_usMaxPacketSize;
		pEPContext->MaxESITPayload = m_usMaxPacketSize;
		break;

	default:
		assert (0);
		break;
	}

	CleanAndInvalidateDataCacheRange ((uintptr) pInputContext, sizeof *pInputContext);

	return pInputContext;
}

void CXHCIEndpoint::FreeInputContext (void)
{
	assert (m_pInputContextBuffer != 0);

	delete [] m_pInputContextBuffer;
	m_pInputContextBuffer = 0;
}

u8 CXHCIEndpoint::ConvertInterval (u8 uchInterval, TUSBSpeed Speed)
{
	if (Speed >= USBSpeedHigh)
	{
		if (uchInterval < 1)
		{
			uchInterval = 1;
		}

		if (uchInterval > 16)
		{
			uchInterval = 16;
		}

		return uchInterval-1;
	}

	unsigned i;
	for (i = 3; i < 11; i++)
	{
		if (125 * (1 << i) >= 1000 * uchInterval)
		{
			break;
		}
	}

	return i;
}
