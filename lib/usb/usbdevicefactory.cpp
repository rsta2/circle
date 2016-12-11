//
// usbdevicefactory.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbdevicefactory.h>
#include <assert.h>

// for factory
#include <circle/usb/usbstandardhub.h>
#include <circle/usb/usbmassdevice.h>
#include <circle/usb/usbkeyboard.h>
#include <circle/usb/usbmouse.h>
#include <circle/usb/usbgamepad.h>
#include <circle/usb/usbprinter.h>
#include <circle/usb/smsc951x.h>
#include <circle/usb/usbbluetooth.h>

CUSBFunction *CUSBDeviceFactory::GetDevice (CUSBFunction *pParent, CString *pName)
{
	assert (pParent != 0);
	assert (pName != 0);
	
	CUSBFunction *pResult = 0;

	if (   pName->Compare ("int9-0-0") == 0
	    || pName->Compare ("int9-0-2") == 0)
	{
		pResult = new CUSBStandardHub (pParent);
	}
	else if (pName->Compare ("int8-6-50") == 0)
	{
		pResult = new CUSBBulkOnlyMassStorageDevice (pParent);
	}
	else if (pName->Compare ("int3-1-1") == 0)
	{
		pResult = new CUSBKeyboardDevice (pParent);
	}
	else if (pName->Compare ("int3-1-2") == 0)
	{
		pResult = new CUSBMouseDevice (pParent);
	}
	else if (pName->Compare ("int3-0-0") == 0)
	{
		pResult = new CUSBGamePadDevice (pParent);
	}
	else if (   pName->Compare ("int7-1-1") == 0
		 || pName->Compare ("int7-1-2") == 0)
	{
		pResult = new CUSBPrinterDevice (pParent);
	}
	else if (pName->Compare ("ven424-ec00") == 0)
	{
		pResult = new CSMSC951xDevice (pParent);
	}
	else if (   pName->Compare ("inte0-1-1") == 0
		 || pName->Compare ("ven50d-65a") == 0)		// Belkin F8T065BF Mini Bluetooth 4.0 Adapter
	{
		pResult = new CUSBBluetoothDevice (pParent);
	}
	// new devices follow

	if (pResult != 0)
	{
		pResult->GetDevice ()->LogWrite (LogNotice, "Using device/interface %s", (const char *) *pName);
	}
	
	delete pName;
	
	return pResult;
}
