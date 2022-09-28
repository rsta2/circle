//
// xhciusbdevice.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/xhciusbdevice.h>
#include <circle/usb/xhcidevice.h>
#include <circle/usb/xhcirootport.h>
#include <circle/usb/usbstandardhub.h>
#include <circle/synchronize.h>
#include <circle/timer.h>
#include <circle/util.h>
#include <circle/debug.h>
#include <assert.h>

static const char From[] = "xhcidev";

CXHCIUSBDevice::CXHCIUSBDevice (CXHCIDevice *pXHCIDevice, TUSBSpeed Speed, CXHCIRootPort *pRootPort)
:	CUSBDevice (pXHCIDevice, Speed, pRootPort),
	m_pXHCIDevice (pXHCIDevice),
	m_pRootPort (pRootPort),
	m_uchSlotID (0),
	m_pDeviceContext (0),
	m_pInputContextBuffer (0)
{
	m_pDeviceContext =
		(TXHCIDeviceContext *) pXHCIDevice->AllocateSharedMem (sizeof (TXHCIDeviceContext));

	// must not clear EP0, because it has already been set from CUSBDevice::CUSBDevice()
	for (unsigned i = 1; i < XHCI_MAX_ENDPOINTS; i++)
	{
		m_pEndpoint[i] = 0;
	}
}

CXHCIUSBDevice::CXHCIUSBDevice (CXHCIDevice *pXHCIDevice, TUSBSpeed Speed,
				CUSBStandardHub *pHub, unsigned nHubPortIndex)
:	CUSBDevice (pXHCIDevice, Speed, pHub, nHubPortIndex),
	m_pXHCIDevice (pXHCIDevice),
	m_pRootPort (0),
	m_uchSlotID (0),
	m_pDeviceContext (0),
	m_pInputContextBuffer (0)
{
	m_pDeviceContext =
		(TXHCIDeviceContext *) pXHCIDevice->AllocateSharedMem (sizeof (TXHCIDeviceContext));

	// must not clear EP0, because it has already been set from CUSBDevice::CUSBDevice()
	for (unsigned i = 1; i < XHCI_MAX_ENDPOINTS; i++)
	{
		m_pEndpoint[i] = 0;
	}
}

CXHCIUSBDevice::~CXHCIUSBDevice (void)
{
	assert (m_pXHCIDevice != 0);

	assert (m_pInputContextBuffer == 0);

	if (m_uchSlotID != 0)
	{
#ifndef NDEBUG
		int nResult =
#endif
			m_pXHCIDevice->GetCommandManager ()->DisableSlot (m_uchSlotID);
		assert (XHCI_CMD_SUCCESS (nResult));

		m_pXHCIDevice->GetSlotManager ()->FreeSlot (m_uchSlotID);

		m_uchSlotID = 0;
	}

	if (m_pDeviceContext != 0)
	{
		m_pXHCIDevice->FreeSharedMem (m_pDeviceContext);

		m_pDeviceContext = 0;
	}

	for (unsigned i = 0; i < XHCI_MAX_ENDPOINTS; i++)
	{
		m_pEndpoint[i] = 0;
	}

	m_pRootPort = 0;
	m_pXHCIDevice = 0;
}

boolean CXHCIUSBDevice::Initialize (void)
{
	if (m_pDeviceContext == 0)
	{
		return FALSE;
	}

	assert (m_pXHCIDevice != 0);
	int nResult = m_pXHCIDevice->GetCommandManager ()->EnableSlot (&m_uchSlotID);
	if (!XHCI_CMD_SUCCESS (nResult))
	{
		CLogger::Get ()->Write (From, LogWarning,
					"Cannot enable slot (%d)", nResult);

		m_uchSlotID = 0;

		return FALSE;
	}

	assert (XHCI_IS_SLOTID (m_uchSlotID));
	m_pXHCIDevice->GetSlotManager ()->AssignDevice (m_uchSlotID, this);

	TXHCIInputContext *pInputContext = GetInputContextAddressDevice ();
	assert (pInputContext != 0);
	nResult = m_pXHCIDevice->GetCommandManager ()->AddressDevice (m_uchSlotID,
								      pInputContext, TRUE);
	FreeInputContext ();
	if (!XHCI_CMD_SUCCESS (nResult))
	{
		CLogger::Get ()->Write (From, LogWarning,
					"Cannot set device address (%d)", nResult);

		return FALSE;
	}

	CTimer::Get ()->MsDelay (50);

	SetAddress (m_uchSlotID);

	return CUSBDevice::Initialize ();
}

boolean CXHCIUSBDevice::EnableHubFunction (void)
{
	TXHCIInputContext *pInputContext = GetInputContextEnableHubFunction ();
	assert (pInputContext != 0);

	int nResult = m_pXHCIDevice->GetCommandManager ()->EvaluateContext (m_uchSlotID,
									    pInputContext);

	FreeInputContext ();

	if (!XHCI_CMD_SUCCESS (nResult))
	{
		CLogger::Get ()->Write (From, LogWarning,
					"Cannot enable hub function (%d)", nResult);

		return FALSE;
	}

	return TRUE;
}

u8 CXHCIUSBDevice::GetSlotID (void) const
{
	assert (XHCI_IS_SLOTID (m_uchSlotID));
	return m_uchSlotID;
}

TXHCIDeviceContext *CXHCIUSBDevice::GetDeviceContext (void)
{
	assert (m_pDeviceContext != 0);
	return m_pDeviceContext;
}

void CXHCIUSBDevice::RegisterEndpoint (u8 uchEndpointID, CXHCIEndpoint *pEndpoint)
{
	assert (XHCI_IS_ENDPOINTID (uchEndpointID));
	assert (pEndpoint != 0);
	m_pEndpoint[uchEndpointID-1] = pEndpoint;
}

void CXHCIUSBDevice::TransferEvent (u8 uchCompletionCode, u32 nTransferLength, u8 uchEndpointID)
{
	assert (XHCI_IS_ENDPOINTID (uchEndpointID));
	assert (m_pEndpoint[uchEndpointID-1] != 0);
	m_pEndpoint[uchEndpointID-1]->TransferEvent (uchCompletionCode, nTransferLength);
}

#ifndef NDEBUG

void CXHCIUSBDevice::DumpStatus (void)
{
	debug_hexdump (m_pDeviceContext,   sizeof (TXHCISlotContext)
					 + 7*sizeof (TXHCIEndpointContext), From);

	for (unsigned i = 0; i < XHCI_MAX_ENDPOINTS; i++)
	{
		if (m_pEndpoint[i] != 0)
		{
			m_pEndpoint[i]->DumpStatus ();
		}
	}
}

#endif

TXHCIInputContext *CXHCIUSBDevice::GetInputContextAddressDevice (void)
{
	assert (m_pInputContextBuffer == 0);
	m_pInputContextBuffer = new u8[sizeof (TXHCIInputContext) + XHCI_PAGE_SIZE-1];
	assert (m_pInputContextBuffer != 0);

	TXHCIInputContext *pInputContext =
		(TXHCIInputContext *) XHCI_ALIGN_PTR (m_pInputContextBuffer, XHCI_PAGE_SIZE);

	memset (pInputContext, 0, sizeof *pInputContext);

	// set input control context
	pInputContext->Control.AddContextFlags = 3;	// set A0 and A1

	// set slot context
	pInputContext->Device.Slot.RootHubPortNumber = GetRootHubPortID ();
	pInputContext->Device.Slot.RouteString = GetRouteString ();
	pInputContext->Device.Slot.Speed = XHCI_USB_SPEED_TO_PSI (GetSpeed ());
	pInputContext->Device.Slot.ContextEntries = 1;

	if (IsSplit ())
	{
		pInputContext->Device.Slot.TTHubSlotID = GetHubAddress ();
		pInputContext->Device.Slot.TTPortNumber = GetHubPortNumber ();

		CUSBDevice *pTTHubDevice = GetTTHubDevice ();
		assert (pTTHubDevice != 0);
		const TUSBHubInfo *pTTHubInfo = pTTHubDevice->GetHubInfo ();
		assert (pTTHubInfo != 0);

		pInputContext->Device.Slot.MTT = pTTHubInfo->HasMultipleTTs ? 1 : 0;
	}

	// set EP0 context
	TXHCIEndpointContext *pEP0Context = &pInputContext->Device.Endpoint[0];

	assert (m_pEndpoint[0] != 0);
	CXHCIRing *pTransferRing = m_pEndpoint[0]->GetTransferRing ();
	assert (pTransferRing != 0);
	pEP0Context->TRDequeuePointer =
		XHCI_TO_DMA (pTransferRing->GetFirstTRB ()) | XHCI_EP_CONTEXT_TR_DEQUEUE_PTR_DCS;

	pEP0Context->Mult = 0;
	pEP0Context->MaxPStreams = 0;
	pEP0Context->Interval = 0;
	pEP0Context->CErr = 3;
	pEP0Context->EPType = XHCI_EP_CONTEXT_EP_TYPE_CONTROL;
	pEP0Context->MaxBurstSize = 0;
	pEP0Context->AverageTRBLength = 8;	// best guess

	switch (GetSpeed ())
	{
	case USBSpeedLow:
	case USBSpeedFull:
		pEP0Context->MaxPacketSize = 8;
		break;

	case USBSpeedHigh:
		pEP0Context->MaxPacketSize = 64;
		break;

	case USBSpeedSuper:
		pEP0Context->MaxPacketSize = 512;
		break;

	default:
		assert (0);
		break;
	}

	CleanAndInvalidateDataCacheRange ((uintptr) pInputContext, sizeof *pInputContext);

	return pInputContext;
}

TXHCIInputContext *CXHCIUSBDevice::GetInputContextEnableHubFunction (void)
{
	assert (m_pInputContextBuffer == 0);
	m_pInputContextBuffer = new u8[sizeof (TXHCIInputContext) + XHCI_PAGE_SIZE-1];
	assert (m_pInputContextBuffer != 0);

	TXHCIInputContext *pInputContext =
		(TXHCIInputContext *) XHCI_ALIGN_PTR (m_pInputContextBuffer, XHCI_PAGE_SIZE);

	assert (m_pDeviceContext != 0);
	memset (&pInputContext->Control, 0, sizeof pInputContext->Control);
	memcpy (&pInputContext->Device, m_pDeviceContext, sizeof pInputContext->Device);

	// set input control context
	pInputContext->Control.AddContextFlags = 1;	// set A0
	pInputContext->Control.DropContextFlags = 1;	// set D0

	// set slot context
	const TUSBHubInfo *pHubInfo = GetHubInfo ();
	assert (pHubInfo != 0);

	pInputContext->Device.Slot.Hub = 1;
	pInputContext->Device.Slot.NumberOfPorts = pHubInfo->NumberOfPorts;

	if (GetSpeed () == USBSpeedHigh)
	{
		pInputContext->Device.Slot.MTT = pHubInfo->HasMultipleTTs ? 1 : 0;
		pInputContext->Device.Slot.TTT = pHubInfo->TTThinkTime;
	}

	CleanAndInvalidateDataCacheRange ((uintptr) pInputContext, sizeof *pInputContext);

	return pInputContext;
}

void CXHCIUSBDevice::FreeInputContext (void)
{
	assert (m_pInputContextBuffer != 0);

	delete [] m_pInputContextBuffer;
	m_pInputContextBuffer = 0;
}
