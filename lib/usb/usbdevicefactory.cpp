//
// usbdevicefactory.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2023  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbhid.h>
#include <circle/synchronize.h>
#include <circle/sysconfig.h>
#include <circle/koptions.h>
#include <circle/logger.h>
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
#include <circle/usb/usbmidihost.h>
#include <circle/usb/usbaudiocontrol.h>
#include <circle/usb/usbaudiostreaming.h>
#include <circle/usb/usbcdcethernet.h>
#include <circle/usb/usbserialcdc.h>
#include <circle/usb/usbserialch341.h>
#include <circle/usb/usbserialcp210x.h>
#include <circle/usb/usbserialpl2303.h>
#include <circle/usb/usbserialft231x.h>
#include <circle/usb/usbtouchscreen.h>

CUSBFunction *CUSBDeviceFactory::GetDevice (CUSBFunction *pParent, CString *pName)
{
	assert (pParent != 0);
	assert (pName != 0);

	const char *pUSBIgnore = CKernelOptions::Get ()->GetUSBIgnore ();
	assert (pUSBIgnore != 0);
	if (pName->Compare (pUSBIgnore) == 0)
	{
		CLogger::Get ()->Write ("ufactory", LogWarning,
					"Ignoring device/interface %s", pUSBIgnore);

		return 0;
	}

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
		CString *pVendor = pParent->GetDevice ()->GetName (DeviceNameVendor);
		assert (pVendor != 0);

		if (pVendor->Compare ("ven3f0-1198") != 0)	// HP USB 1000dpi Laser Mouse
		{
			pResult = new CUSBKeyboardDevice (pParent);
		}

		delete pVendor;
	}
	else if (pName->Compare ("int3-1-2") == 0)
	{
		pResult = new CUSBMouseDevice (pParent);
	}
	else if (   pName->Compare ("int3-0-0") == 0
		 || pName->Compare ("int3-0-2") == 0)
	{
		CString *pVendor = pParent->GetDevice ()->GetName (DeviceNameVendor);
		assert (pVendor != 0);

		if (pVendor->Compare ("ven5ac-21e") != 0)	// Apple Aluminum Mini Keyboard
		{
			pResult = GetGenericHIDDevice (pParent);
		}

		delete pVendor;
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
		pResult = new CUSBMIDIHostDevice (pParent);
	}
#if RASPPI >= 4
	else if (   pName->Compare ("int1-1-0") == 0
		 || pName->Compare ("int1-1-20") == 0)
	{
		pResult = new CUSBAudioControlDevice (pParent);
	}
	else if (   pName->Compare ("int1-2-0") == 0
		 || pName->Compare ("int1-2-20") == 0)
	{
		pResult = new CUSBAudioStreamingDevice (pParent);
	}
#endif
	else if (pName->Compare ("int2-6-0") == 0)
	{
		pResult = new CUSBCDCEthernetDevice (pParent);
	}
	else if (   pName->Compare ("int2-2-0") == 0
		 || pName->Compare ("int2-2-1") == 0)
	{
		pResult = new CUSBSerialCDCDevice (pParent);
	}
	else if (FindDeviceID (pName, CUSBSerialCH341Device::GetDeviceIDTable ()))
	{
		pResult = new CUSBSerialCH341Device (pParent);
	}
	else if (FindDeviceID (pName, CUSBSerialCP210xDevice::GetDeviceIDTable ()))
	{
		pResult = new CUSBSerialCP210xDevice (pParent);
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

CUSBFunction *CUSBDeviceFactory::GetGenericHIDDevice (CUSBFunction *pParent)
{
	// Must copy parent function here, because we consume the HID report descriptor,
	// which is requested again later by the HID Use Page specific driver class.
	CUSBFunction TempFunction (pParent);

	TUSBHIDDescriptor *pHIDDesc =
		(TUSBHIDDescriptor *) TempFunction.GetDescriptor (DESCRIPTOR_HID);
	if (   pHIDDesc == 0
	    || pHIDDesc->wReportDescriptorLength == 0)
	{
		TempFunction.ConfigurationError ("usbhid");

		return 0;
	}

	u16 usReportDescriptorLength = pHIDDesc->wReportDescriptorLength;
	DMA_BUFFER (u8, ReportDescriptor, usReportDescriptorLength);

	if (   TempFunction.GetHost ()->GetDescriptor (
			TempFunction.GetEndpoint0 (),
			pHIDDesc->bReportDescriptorType, DESCRIPTOR_INDEX_DEFAULT,
			ReportDescriptor, usReportDescriptorLength,
			REQUEST_IN | REQUEST_TO_INTERFACE, TempFunction.GetInterfaceNumber ())
	    != usReportDescriptorLength)
	{
		TempFunction.GetDevice ()->LogWrite (LogError, "Cannot get HID report descriptor");

		return 0;
	}

	// If we find a Usage Page (Digitizer) item anywhere in the HID report descriptor,
	// then use the touch screen driver, otherwise the gamepad standard driver.
	const u8 *pDesc = ReportDescriptor;
	for (u16 nDescSize = usReportDescriptorLength; nDescSize;)
	{
		u8 ucItem = *pDesc++;
		nDescSize--;

		u32 nArg;

		switch (ucItem & 0x03)
		{
		case 0:
			nArg = 0;
			break;

		case 1:
			nArg = *pDesc++;
			nDescSize--;
			break;

		case 2:
			nArg  = *pDesc++;
			nArg |= *pDesc++ << 8;
			nDescSize -= 2;
			break;

		case 3:
			nArg  = *pDesc++;
			nArg |= *pDesc++ << 8;
			nArg |= *pDesc++ << 16;
			nArg |= *pDesc++ << 24;
			nDescSize -= 4;
			break;
		}

		ucItem &= 0xFC;

		if (   ucItem == 0x04		// Usage Page (Digitizer)
		    && nArg == 0x0D)
		{
			return new CUSBTouchScreenDevice (pParent);
		}
	}

	return new CUSBGamePadStandardDevice (pParent);
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
