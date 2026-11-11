//
// usbdevicefactory.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2026  R. Stange <rsta2@gmx.net>
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
#ifndef _circle_usb_usbdevicefactory_h
#define _circle_usb_usbdevicefactory_h

#include <circle/usb/usbfunction.h>
#include <circle/string.h>
#include <circle/types.h>

#define USB_DEVICE(vendorid, deviceid)		vendorid, deviceid

struct TUSBDeviceID
{
	u16	usVendorID;
	u16	usDeviceID;
};

class CUSBDeviceFactory
{
public:
	static CUSBFunction *GetDevice (CUSBFunction *pParent, CString *pName);

private:
	// for "int3-0-0" devices
	static CUSBFunction *GetGenericHIDDevice (CUSBFunction *pParent);

	static boolean FindDeviceID (CString *pName, const TUSBDeviceID *pIDTable);

public:
	// allow creating of user devices
	typedef CUSBFunction *TUserFactory (CUSBFunction *pParent, CString *pName);

	/// \param pUserFactory User factory function (0 to disable)
	static void SetUserFactory (TUserFactory *pUserFactory);

private:
	static TUserFactory *s_pUserFactory;
};

#endif
