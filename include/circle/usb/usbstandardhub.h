//
// usbstandardhub.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2019  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbstandardhub_h
#define _circle_usb_usbstandardhub_h

#include <circle/usb/usb.h>
#include <circle/usb/usbhub.h>
#include <circle/usb/usbfunction.h>
#include <circle/usb/usbendpoint.h>
#include <circle/usb/usbdevice.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/types.h>

class CUSBStandardHub : public CUSBFunction
{
public:
	CUSBStandardHub (CUSBFunction *pFunction);
	~CUSBStandardHub (void);
	
	boolean Initialize (void);
	boolean Configure (void);

	boolean ReScanDevices (void);
	boolean RemoveDevice (unsigned nPortIndex);	// nPortIndex is 0-based

	boolean DisablePort (unsigned nPortIndex);	// nPortIndex is 0-based

#if RASPPI >= 4
	const TUSBHubInfo *GetHubInfo (void) const;
#endif

private:
	boolean EnumeratePorts (void);

private:
	TUSBHubDescriptor *m_pHubDesc;

	CUSBEndpoint *m_pInterruptEndpoint;

	unsigned m_nPorts;
	boolean m_bPowerIsOn;
	CUSBDevice *m_pDevice[USB_HUB_MAX_PORTS];
	TUSBPortStatus *m_pStatus[USB_HUB_MAX_PORTS];
	boolean m_bPortConfigured[USB_HUB_MAX_PORTS];

#if RASPPI >= 4
	TUSBHubInfo *m_pHubInfo;
#endif

	static unsigned s_nDeviceNumber;
};

#endif
