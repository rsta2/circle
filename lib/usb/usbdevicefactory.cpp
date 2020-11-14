//
// usbdevicefactory.cpp
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
#include <circle/usb/usbdevicefactory.h>
#include <assert.h>

// for factory
#include <circle/usb/usbstandardhub.h>
#include <circle/usb/usbmassdevice.h>
#include <circle/usb/usbkeyboard.h>
#include <circle/usb/usbmouse.h>
#include <circle/usb/usbgamepadstandard.h>
#include <circle/usb/usbgamepadps3.h>
#include <circle/usb/usbgamepadps4.h>
#include <circle/usb/usbgamepadxbox360.h>
#include <circle/usb/usbgamepadxboxone.h>
#include <circle/usb/usbgamepadswitchpro.h>
#include <circle/usb/usbprinter.h>
#include <circle/usb/smsc951x.h>
#include <circle/usb/lan7800.h>
#include <circle/usb/usbbluetooth.h>
#include <circle/usb/usbmidi.h>
#include <circle/usb/usbcdcethernet.h>
#include <circle/usb/usbserialcdc.h>
#include <circle/usb/usbserialch341.h>
#include <circle/usb/usbserialcp2102.h>
#include <circle/usb/usbserialpl2303.h>
#include <circle/usb/usbserialft231x.h>

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
		pResult = new CUSBGamePadStandardDevice (pParent);
	}
	else if (pName->Compare ("ven54c-268") == 0)
	{
		pResult = new CUSBGamePadPS3Device (pParent);
	}
	else if (   pName->Compare ("ven54c-5c4") == 0
		 || pName->Compare ("ven54c-9cc") == 0)
	{
		pResult = new CUSBGamePadPS4Device (pParent);
	}
	else if (   pName->Compare ("ven45e-28e") == 0
		 || pName->Compare ("ven45e-28f") == 0)
	{
		pResult = new CUSBGamePadXbox360Device (pParent);
	}
	else if (   pName->Compare ("ven45e-2d1") == 0		// XBox One Controller
		 || pName->Compare ("ven45e-2dd") == 0		// XBox One Controller (FW 2015)
		 || pName->Compare ("ven45e-2e3") == 0		// XBox One Elite Controller
		 || pName->Compare ("ven45e-2ea") == 0)		// XBox One S Controller
	{
		pResult = new CUSBGamePadXboxOneDevice (pParent);
	}
	else if (pName->Compare ("ven57e-2009") == 0)
	{
		pResult = new CUSBGamePadSwitchProDevice (pParent);
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
	else if (pName->Compare ("ven424-7800") == 0)
	{
		pResult = new CLAN7800Device (pParent);
	}
	else if (   pName->Compare ("inte0-1-1") == 0
		 || pName->Compare ("ven50d-65a") == 0)		// Belkin F8T065BF Mini Bluetooth 4.0 Adapter
	{
		pResult = new CUSBBluetoothDevice (pParent);
	}
	else if (   pName->Compare ("int1-3-0") == 0
		 || pName->Compare ("ven582-12a") == 0)		// Roland UM-ONE MIDI interface
	{
		pResult = new CUSBMIDIDevice (pParent);
	}
	else if (pName->Compare ("int2-6-0") == 0)
	{
		pResult = new CUSBCDCEthernetDevice (pParent);
	}
	else if (pName->Compare ("int2-2-1") == 0)
	{
		pResult = new CUSBSerialCDCDevice (pParent);
	}
	else if (FindDeviceID (pName, CUSBSerialCH341Device::GetDeviceIDTable ()))
	{
		pResult = new CUSBSerialCH341Device (pParent);
	}
	else if (FindDeviceID (pName, CUSBSerialCP2102Device::GetDeviceIDTable ()))
	{
		pResult = new CUSBSerialCP2102Device (pParent);
	}
	else if (FindDeviceID (pName, CUSBSerialPL2303Device::GetDeviceIDTable ()))
	{
		pResult = new CUSBSerialPL2303Device (pParent);
	}
	else if (FindDeviceID (pName, CUSBSerialFT231XDevice::GetDeviceIDTable ()))
	{
		pResult = new CUSBSerialFT231XDevice (pParent);
	}
	// new devices follow

	if (pResult != 0)
	{
		pResult->GetDevice ()->LogWrite (LogNotice, "Using device/interface %s", (const char *) *pName);
	}

	delete pName;

	return pResult;
}

boolean CUSBDeviceFactory::FindDeviceID (CString *pName, const TUSBDeviceID *pIDTable)
{
	while (   pIDTable->usVendorID != 0
	       || pIDTable->usDeviceID != 0)
	{
		CString String;
		String.Format ("ven%x-%x", (unsigned) pIDTable->usVendorID,
					   (unsigned) pIDTable->usDeviceID);

		if (pName->Compare (String) == 0)
		{
			return TRUE;
		}

		pIDTable++;
	}

	return FALSE;
}
