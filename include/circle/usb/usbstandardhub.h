//
// usbstandardhub.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#ifndef _usbstandardhub_h
#define _usbstandardhub_h

#include <circle/usb/usb.h>
#include <circle/usb/usbhub.h>
#include <circle/usb/usbfunction.h>
#include <circle/usb/usbdevice.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/types.h>

class CUSBStandardHub : public CUSBFunction
{
public:
	CUSBStandardHub (CUSBFunction *pFunction);
	~CUSBStandardHub (void);
	
	boolean Configure (void);

private:
	boolean EnumeratePorts (void);

private:
	TUSBHubDescriptor *m_pHubDesc;

	unsigned m_nPorts;
	CUSBDevice *m_pDevice[USB_HUB_MAX_PORTS];
	TUSBPortStatus *m_pStatus[USB_HUB_MAX_PORTS];

	static unsigned s_nDeviceNumber;
};

#endif
