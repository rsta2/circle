//
// xhcirootport.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_xhcirootport_h
#define _circle_usb_xhcirootport_h

#include <circle/usb/usbhcirootport.h>
#include <circle/usb/xhcimmiospace.h>
#include <circle/usb/xhciusbdevice.h>
#include <circle/usb/xhci.h>
#include <circle/usb/usb.h>
#include <circle/spinlock.h>
#include <circle/types.h>

class CXHCIDevice;

class CXHCIRootPort : public CUSBHCIRootPort	/// Encapsulates an xHCI USB root port
{
public:
	CXHCIRootPort (u8 uchPortID, CXHCIDevice *pXHCIDevice);
	~CXHCIRootPort (void);

	boolean Initialize (void);
	boolean Configure (void);

	u8 GetPortID (void) const;
	TUSBSpeed GetPortSpeed (void);		// port must be enabled before

	boolean ReScanDevices (void);
	boolean RemoveDevice (void);

	void HandlePortStatusChange (void);

	void StatusChanged (void);

#ifndef NDEBUG
	void DumpStatus (void);
#endif

private:
	boolean IsConnected (void);

	boolean Reset (unsigned nTimeoutUsecs);			// for USB2 ports
	boolean WaitForU0State (unsigned nTimeoutUsecs);	// for USB3 ports

	boolean PowerOffOnOverCurrent (void);	// returns TRUE if powered off

private:
	unsigned	 m_nPortIndex;
	CXHCIDevice	*m_pXHCIDevice;
	CXHCIMMIOSpace	*m_pMMIO;

	CXHCIUSBDevice	*m_pUSBDevice;

	CSpinLock	 m_SpinLock;
};

#endif
