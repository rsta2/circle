//
// xhciusbdevice.h
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
#ifndef _circle_usb_xhciusbdevice_h
#define _circle_usb_xhciusbdevice_h

#include <circle/usb/usbdevice.h>
#include <circle/usb/usb.h>
#include <circle/usb/xhciendpoint.h>
#include <circle/usb/xhci.h>
#include <circle/types.h>

class CXHCIDevice;
class CXHCIRootPort;
class CUSBStandardHub;

class CXHCIUSBDevice : public CUSBDevice	/// Encapsulates an USB device, attached xHC
{
public:
	CXHCIUSBDevice (CXHCIDevice *pXHCIDevice, TUSBSpeed Speed, CXHCIRootPort *pRootPort);
	CXHCIUSBDevice (CXHCIDevice *pXHCIDevice, TUSBSpeed Speed,
			CUSBStandardHub *pHub, unsigned nHubPortIndex);
	~CXHCIUSBDevice (void);

	boolean Initialize (void);
	boolean EnableHubFunction (void);

	u8 GetSlotID (void) const;
	TXHCIDeviceContext *GetDeviceContext (void);

	void RegisterEndpoint (u8 uchEndpointID, CXHCIEndpoint *pEndpoint);

	void TransferEvent (u8 uchCompletionCode, u32 nTransferLength, u8 uchEndpointID);

#ifndef NDEBUG
	void DumpStatus (void);
#endif

private:
	TXHCIInputContext *GetInputContextAddressDevice (void);
	TXHCIInputContext *GetInputContextEnableHubFunction (void);
	void FreeInputContext (void);

private:
	CXHCIDevice	*m_pXHCIDevice;
	CXHCIRootPort	*m_pRootPort;		// the root port, this device is connected to

	u8 m_uchSlotID;
	TXHCIDeviceContext *m_pDeviceContext;

	CXHCIEndpoint *m_pEndpoint[XHCI_MAX_ENDPOINTS];

	u8 *m_pInputContextBuffer;
};

#endif
