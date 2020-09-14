//
// dwhcirootport.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_dwhcirootport_h
#define _circle_usb_dwhcirootport_h

#include <circle/usb/usbhcirootport.h>
#include <circle/usb/usbdevice.h>
#include <circle/types.h>

class CDWHCIDevice;

class CDWHCIRootPort : public CUSBHCIRootPort
{
public:
	CDWHCIRootPort (CDWHCIDevice *pHost);
	~CDWHCIRootPort (void);

	boolean Initialize (void);

	boolean ReScanDevices (void);
	boolean RemoveDevice (void);

	void HandlePortStatusChange (void);

private:
	CDWHCIDevice *m_pHost;

	CUSBDevice *m_pDevice;
};

#endif
