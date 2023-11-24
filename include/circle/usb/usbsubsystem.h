//
// usbsubsystem.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbsubsystem_h
#define _circle_usb_usbsubsystem_h

#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/southbridge.h>
#include <circle/usb/usb.h>
#include <circle/usb/usbendpoint.h>
#include <circle/usb/usbcontroller.h>
#include <circle/usb/xhcisharedmemallocator.h>
#include <circle/usb/xhcidevice.h>
#include <circle/types.h>

class CUSBSubSystem : public CUSBController	/// USB sub-system of the Raspberry Pi 5
{
public:
	CUSBSubSystem (CInterruptSystem *pInterruptSystem, CTimer *pTimer,
		       boolean bPlugAndPlay = FALSE);
	~CUSBSubSystem (void);

	boolean Initialize (boolean bScanDevices = TRUE) override;

	boolean UpdatePlugAndPlay (void) override;

	int GetDescriptor (CUSBEndpoint *pEndpoint,
			   unsigned char ucType, unsigned char ucIndex,
			   void *pBuffer, unsigned nBufSize,
			   unsigned char ucRequestType = REQUEST_IN,
			   unsigned short wIndex = 0);

	boolean IsPlugAndPlay (void) const;

	static boolean IsActive (void);

	static CUSBSubSystem *Get (void);

#ifndef NDEBUG
	void DumpStatus (void);
#endif

private:
	static const unsigned NumDevices = 2;

private:
	boolean m_bPlugAndPlay;

	CSouthbridge m_Southbridge;

	CXHCISharedMemAllocator m_SharedMemAllocator;

	CXHCIDevice *m_pXHCIDevice[NumDevices];

	static CUSBSubSystem *s_pThis;
};

#endif
