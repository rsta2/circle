//
// xhciendpoint.h
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
#ifndef _circle_usb_xhciendpoint_h
#define _circle_usb_xhciendpoint_h

#include <circle/usb/xhcimmiospace.h>
#include <circle/usb/xhciring.h>
#include <circle/usb/xhci.h>
#include <circle/usb/usb.h>
#include <circle/spinlock.h>
#include <circle/types.h>

class CXHCIDevice;
class CXHCIUSBDevice;
class CUSBRequest;

class CXHCIEndpoint	/// Encapsulates a single endpoint of an USB device for the xHCI driver
{
public:
	CXHCIEndpoint (CXHCIUSBDevice *pDevice, CXHCIDevice *pXHCIDevice);		// EP0
	CXHCIEndpoint (CXHCIUSBDevice *pDevice, const TUSBEndpointDescriptor *pDesc,	// EP1-30
		       CXHCIDevice *pXHCIDevice);
	~CXHCIEndpoint (void);

	boolean IsValid (void);

	CXHCIRing *GetTransferRing (void);

	boolean SetMaxPacketSize (u32 nMaxPacketSize);

	boolean Transfer (CUSBRequest *pURB, unsigned nTimeoutMs);
	boolean TransferAsync (CUSBRequest *pURB, unsigned nTimeoutMs);

	void TransferEvent (u8 uchCompletionCode, u32 nTransferLength);

#ifndef NDEBUG
	void DumpStatus (void);
#endif

private:
	static void CompletionRoutine (CUSBRequest *pURB, void *pParam, void *pContext);

	// Cycle bit and Interrupter Target are set automatically
	boolean EnqueueTRB (u32 nControl, u32 nStatus = 0,
			    u32 nParameter1 = 0, u32 nParameter2 = 0);

	TXHCIInputContext *GetInputContextSetMaxPacketSize (void);
	TXHCIInputContext *GetInputContextConfigureEndpoint (void);
	void FreeInputContext (void);

	static u8 ConvertInterval (u8 uchInterval, TUSBSpeed Speed);

private:
	CXHCIUSBDevice *m_pDevice;
	CXHCIDevice    *m_pXHCIDevice;
	CXHCIMMIOSpace *m_pMMIO;

	boolean		 m_bValid;
	CXHCIRing	*m_pTransferRing;

	// from endpoint descriptor
	u8		 m_uchEndpointAddress;
	u8		 m_uchAttributes;
	u16		 m_usMaxPacketSize;
	u8		 m_uchInterval;

	u8		 m_uchEndpointID;
	u8		 m_uchEndpointType;

	CUSBRequest	*m_pURB[2];
	volatile boolean m_bTransferCompleted;

	u8		*m_pInputContextBuffer;

	CSpinLock	 m_SpinLock;
};

#endif
