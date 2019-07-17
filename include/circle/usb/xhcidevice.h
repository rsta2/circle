//
// xhcidevice.h
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
#ifndef _circle_usb_xhcidevice_h
#define _circle_usb_xhcidevice_h

#include <circle/usb/usbhostcontroller.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/usb/usbrequest.h>
#include <circle/types.h>

class CXHCIDevice : public CUSBHostController
{
public:
	CXHCIDevice (CInterruptSystem *pInterruptSystem, CTimer *pTimer);
	~CXHCIDevice (void);

	boolean Initialize (void);

	void ReScanDevices (void);

	boolean SubmitBlockingRequest (CUSBRequest *pURB, unsigned nTimeoutMs = USB_TIMEOUT_NONE);
	boolean SubmitAsyncRequest (CUSBRequest *pURB, unsigned nTimeoutMs = USB_TIMEOUT_NONE);
};

#endif
