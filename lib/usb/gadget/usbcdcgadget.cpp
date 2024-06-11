//
// usbcdcgadget.cpp
//
// This file by Sebastien Nicolas <seba1978@gmx.de>
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023-2024  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/gadget/usbcdcgadget.h>
#include <circle/usb/gadget/usbcdcgadgetendpoint.h>
#include <circle/sysconfig.h>
#include <assert.h>

const TUSBDeviceDescriptor CUSBCDCGadget::s_DeviceDescriptor =
{
	sizeof (TUSBDeviceDescriptor),
	DESCRIPTOR_DEVICE,
	0x200,				// bcdUSB
	2, 0, 0,			// bDeviceClass
	64,				// wMaxPacketSize0
	USB_GADGET_VENDOR_ID,
	USB_GADGET_DEVICE_ID_SERIAL_CDC,
	0x100,				// bcdDevice
	1, 2, 0,			// strings
	1
};

const CUSBCDCGadget::TUSBCDCGadgetConfigurationDescriptor
	CUSBCDCGadget::s_ConfigurationDescriptor =
{
	{
		sizeof (TUSBConfigurationDescriptor),
		DESCRIPTOR_CONFIGURATION,
		sizeof s_ConfigurationDescriptor,
		2,			// bNumInterfaces
		1,
		0,
		0x80,			// bmAttributes (bus-powered)
		100 / 2			// bMaxPower (500mA) todo back to 500
	},
	{
		sizeof (TUSBInterfaceDescriptor),
		DESCRIPTOR_INTERFACE,
		0,			// bInterfaceNumber
		0,			// bAlternateSetting
		1,			// bNumEndpoints
		2, 2, 1,	// bInterfaceClass, SubClass, Protocol
		0			// iInterface
	},
	{
		5,			// bFunctionLength1
		DESCRIPTOR_CS_INTERFACE,
		0,			// bDescriptorSubtype1 = header functional descriptor
		0x110,		// bcdCDC

		4,			// bFunctionLength2
		DESCRIPTOR_CS_INTERFACE,
	    2,			// bDescriptorSubtype2 = acm functional descriptor
	    0x0,		// bmCapabilities1

		5,			// bFunctionLength3
		DESCRIPTOR_CS_INTERFACE,
	    6,			// bDescriptorSubtype3 = union functional descriptor
	    0,			// bMasterInterface
		1,			// bSubordinateInterface0

		/*
		5,			// bFunctionLength4
		DESCRIPTOR_CS_INTERFACE,
	    1,			// bDescriptorSubtype4 = call management functional descriptor
	    3,			// bmCapabilities2
		1,			// bDataInterface
		*/
	},
	{
		sizeof (TUSBEndpointDescriptor),
		DESCRIPTOR_ENDPOINT,
		EPNotif | 0x80,
		//2,			// bmAttributes (Bulk)
		//todo circle does not support interrupt endpoints but then Linux's CDC driver keeps segfaulting with a bulk notif interrupt...
		// this is OK because that interrupt is never used anyway - not even created (see below)
		// note: we do need this unused notif interrupt or linux does not create /dev/ttyACM0
		3,			// bmAttributes (Interrupt)
		10,			// wMaxPacketSize
		11,		// bInterval
	},
	{
		sizeof (TUSBInterfaceDescriptor),
		DESCRIPTOR_INTERFACE,
		1,			// bInterfaceNumber
		0,			// bAlternateSetting
		2,			// bNumEndpoints
		0xA, 0, 0,	// bInterfaceClass, SubClass, Protocol
		0			// iInterface
	},
	{
		sizeof (TUSBEndpointDescriptor),
		DESCRIPTOR_ENDPOINT,
		EPOut,
		2,			// bmAttributes (Bulk)
		512,		// wMaxPacketSize
		0,			// bInterval
	},
	{
		sizeof (TUSBEndpointDescriptor),
		DESCRIPTOR_ENDPOINT,
		EPIn | 0x80,
		2,			// bmAttributes (Bulk)
		512,		// wMaxPacketSize
		0,			// bInterval
	}
};

const char *const CUSBCDCGadget::s_StringDescriptor[] =
{
	"\x04\x03\x09\x04",		// Language ID
	"Circle",
	"CDC Gadget"
};

CUSBCDCGadget::CUSBCDCGadget (CInterruptSystem *pInterruptSystem)
:	CDWUSBGadget (pInterruptSystem, HighSpeed),
	m_pInterface (nullptr),
	m_pEP {nullptr, nullptr, nullptr, nullptr}
{
}

CUSBCDCGadget::~CUSBCDCGadget (void)
{
	assert (0);
}

const void *CUSBCDCGadget::GetDescriptor (u16 wValue, u16 wIndex, size_t *pLength)
{
	assert (pLength);

	u8 uchDescIndex = wValue & 0xFF;

	switch (wValue >> 8)
	{
	case DESCRIPTOR_DEVICE:
		if (!uchDescIndex)
		{
			*pLength = sizeof s_DeviceDescriptor;
			return &s_DeviceDescriptor;
		}
		break;

	case DESCRIPTOR_CONFIGURATION:
		if (!uchDescIndex)
		{
			*pLength = sizeof s_ConfigurationDescriptor;
			return &s_ConfigurationDescriptor;
		}
		break;

	case DESCRIPTOR_STRING:
		if (!uchDescIndex)
		{
			*pLength = (u8) s_StringDescriptor[0][0];
			return s_StringDescriptor[0];
		}
		else if (uchDescIndex < sizeof s_StringDescriptor / sizeof s_StringDescriptor[0])
		{
			return ToStringDescriptor (s_StringDescriptor[uchDescIndex], pLength);
		}
		break;

	default:
		break;
	}

	return nullptr;
}

void CUSBCDCGadget::AddEndpoints (void)
{
	/*not necessary, which is fortunate because circle does not support Interrupt endpoints
	assert (!m_pEP[EPNotif]);
	m_pEP[EPNotif] = new CUSBCDCGadgetEndpoint (
				reinterpret_cast<const TUSBEndpointDescriptor *> (
					&s_ConfigurationDescriptor.EndpointNotif), this);
	assert (m_pEP[EPNotif]);
	*/

	assert (!m_pEP[EPOut]);
	m_pEP[EPOut] = new CUSBCDCGadgetEndpoint (
				reinterpret_cast<const TUSBEndpointDescriptor *> (
					&s_ConfigurationDescriptor.EndpointOut), this);
	assert (m_pEP[EPOut]);

	assert (!m_pEP[EPIn]);
	m_pEP[EPIn] = new CUSBCDCGadgetEndpoint (
				reinterpret_cast<const TUSBEndpointDescriptor *> (
					&s_ConfigurationDescriptor.EndpointIn), this);
	assert (m_pEP[EPIn]);
}

void CUSBCDCGadget::CreateDevice (void)
{
	assert (!m_pInterface);
	m_pInterface = new CUSBSerialDevice;
	assert (m_pInterface);

	assert (m_pEP[EPOut]);
	m_pEP[EPOut]->AttachInterface (m_pInterface);

	assert (m_pEP[EPIn]);
	m_pEP[EPIn]->AttachInterface (m_pInterface);
}

void CUSBCDCGadget::OnSuspend (void)
{
	delete m_pInterface;
	m_pInterface = nullptr;

	/*not necessary
	delete m_pEP[EPNotif];
	m_pEP[EPNotif] = nullptr;
	*/

	delete m_pEP[EPOut];
	m_pEP[EPOut] = nullptr;

	delete m_pEP[EPIn];
	m_pEP[EPIn] = nullptr;
}

const void *CUSBCDCGadget::ToStringDescriptor (const char *pString, size_t *pLength)
{
	assert (pString);

	size_t nLength = 2;
	for (u8 *p = m_StringDescriptorBuffer+2; *pString; pString++)
	{
		assert (nLength < sizeof m_StringDescriptorBuffer-1);

		*p++ = (u8) *pString;		// convert to UTF-16
		*p++ = '\0';

		nLength += 2;
	}

	m_StringDescriptorBuffer[0] = (u8) nLength;
	m_StringDescriptorBuffer[1] = DESCRIPTOR_STRING;

	assert (pLength);
	*pLength = nLength;

	return m_StringDescriptorBuffer;
}
