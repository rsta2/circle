//
// usbmidigadget.cpp
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
#include "usbmidigadget.h"
#include "usbmidigadgetendpoint.h"
#include <circle/sysconfig.h>
#include <circle/logger.h>
#include <assert.h>

LOGMODULE ("midigadget");

const TUSBDeviceDescriptor CUSBMIDIGadget::s_DeviceDescriptor =
{
	sizeof (TUSBDeviceDescriptor),
	DESCRIPTOR_DEVICE,
	0x100,				// bcdUSB
	0, 0, 0,
	64,				// wMaxPacketSize0
	USB_GADGET_VENDOR_ID,
	USB_GADGET_DEVICE_ID_BASE,
	0x100,				// bcdDevice
	1, 2, 0,			// strings
	1
};

const CUSBMIDIGadget::TUSBMIDIGadgetConfigurationDescriptor
	CUSBMIDIGadget::s_ConfigurationDescriptor =
{
	{
		sizeof (TUSBConfigurationDescriptor),
		DESCRIPTOR_CONFIGURATION,
		sizeof s_ConfigurationDescriptor,
		2,			// bNumInterfaces
		1,
		0,
		0x80,			// bmAttributes (bus-powered)
		100 / 2			// bMaxPower (100mA)
	},
	{
		sizeof (TUSBInterfaceDescriptor),
		DESCRIPTOR_INTERFACE,
		0,			// bInterfaceNumber
		0,			// bAlternateSetting
		0,			// bNumEndpoints
		1, 1, 0,		// bInterfaceClass, SubClass, Protocol
		0
	},
	{
		sizeof (TUSBAudioControlInterfaceDescriptorHeader),
		DESCRIPTOR_CS_INTERFACE,
		USB_AUDIO_CTL_IFACE_SUBTYPE_HEADER,
		USB_AUDIO_CTL_IFACE_BCDADC_100,
		sizeof (TUSBAudioControlInterfaceDescriptorHeader),
		1,
		1
	},
	{
		sizeof (TUSBInterfaceDescriptor),
		DESCRIPTOR_INTERFACE,
		1,			// bInterfaceNumber
		0,			// bAlternateSetting
		2,			// bNumEndpoints
		1, 3, 0,		// bInterfaceClass, SubClass, Protocol
		0
	},
	{
		sizeof (TUSBMIDIStreamingInterfaceDescriptorHeader),
		DESCRIPTOR_CS_INTERFACE,
		USB_MIDI_STREAMING_IFACE_SUBTYPE_HEADER,
		USB_MIDI_STREAMING_IFACE_BCDADC_100,
		  sizeof (TUSBMIDIStreamingInterfaceDescriptorHeader)
		+ sizeof (TUSBMIDIStreamingInterfaceDescriptorInJack)*2
		+ sizeof (TUSBMIDIStreamingInterfaceDescriptorOutJack)*2
		+ sizeof (TUSBAudioEndpointDescriptor)*2
		+ sizeof (TUSBMIDIStreamingEndpointDescriptor)*2
	},
	{
		sizeof (TUSBMIDIStreamingInterfaceDescriptorInJack),
		DESCRIPTOR_CS_INTERFACE,
		USB_MIDI_STREAMING_IFACE_SUBTYPE_MIDI_IN_JACK,
		USB_MIDI_STREAMING_IFACE_JACKTYPE_EMBEDDED,
		1,			// bJackID
		0
	},
	{
		sizeof (TUSBMIDIStreamingInterfaceDescriptorInJack),
		DESCRIPTOR_CS_INTERFACE,
		USB_MIDI_STREAMING_IFACE_SUBTYPE_MIDI_IN_JACK,
		USB_MIDI_STREAMING_IFACE_JACKTYPE_EXTERNAL,
		2,			// bJackID
		0
	},
	{
		sizeof (TUSBMIDIStreamingInterfaceDescriptorOutJack),
		DESCRIPTOR_CS_INTERFACE,
		USB_MIDI_STREAMING_IFACE_SUBTYPE_MIDI_OUT_JACK,
		USB_MIDI_STREAMING_IFACE_JACKTYPE_EMBEDDED,
		3,			// bJackID
		1,			// bNrInputPins
		{2},			// baSourceID[]
		{1},			// baSourcePin[]
		0
	},
	{
		sizeof (TUSBMIDIStreamingInterfaceDescriptorOutJack),
		DESCRIPTOR_CS_INTERFACE,
		USB_MIDI_STREAMING_IFACE_SUBTYPE_MIDI_OUT_JACK,
		USB_MIDI_STREAMING_IFACE_JACKTYPE_EXTERNAL,
		4,			// bJackID
		1,			// bNrInputPins
		{1},			// baSourceID[]
		{1},			// baSourcePin[]
		0
	},
	{
		sizeof (TUSBAudioEndpointDescriptor),
		DESCRIPTOR_ENDPOINT,
		EPOut,
		2,			// bmAttributes (Bulk)
		64,			// wMaxPacketSize
		0,
		0, 0
	},
	{
		sizeof (TUSBMIDIStreamingEndpointDescriptor),
		DESCRIPTOR_CS_ENDPOINT,
		USB_MIDI_STREAMING_SUBTYPE_GENERAL,
		1,			// bNumEmbMIDIJack
		{1}			// bAssocJackIDs[]
	},
	{
		sizeof (TUSBAudioEndpointDescriptor),
		DESCRIPTOR_ENDPOINT,
		EPIn | 0x80,
		2,			// bmAttributes (Bulk)
		64,			// wMaxPacketSize
		0,
		0, 0
	},
	{
		sizeof (TUSBMIDIStreamingEndpointDescriptor),
		DESCRIPTOR_CS_ENDPOINT,
		USB_MIDI_STREAMING_SUBTYPE_GENERAL,
		1,			// bNumEmbMIDIJack
		{3}			// bAssocJackIDs[]
	}
};

const char *const CUSBMIDIGadget::s_StringDescriptor[] =
{
	"\x04\x03\x09\x04",
	"\x0E\x03""C\0i\0r\0c\0l\0e\0",
	"\x18\x03M\0I\0D\0I\0 \0G\0a\0d\0g\0e\0t\0"
};

CUSBMIDIGadget::CUSBMIDIGadget (CInterruptSystem *pInterruptSystem)
:	CDWUSBGadget (pInterruptSystem, FullSpeed),
	m_pEP {nullptr, nullptr, nullptr}
{
}

CUSBMIDIGadget::~CUSBMIDIGadget (void)
{
	assert (0);
}

boolean CUSBMIDIGadget::Initialize (void)
{
	return CDWUSBGadget::Initialize ();
}

const void *CUSBMIDIGadget::GetDescriptor (u16 wValue, u16 wIndex, size_t *pLength) const
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
		if (uchDescIndex < sizeof s_StringDescriptor / sizeof s_StringDescriptor[0])
		{
			*pLength = (u8) s_StringDescriptor[uchDescIndex][0];
			return s_StringDescriptor[uchDescIndex];
		}
		break;

	default:
		break;
	}

	return nullptr;
}

void CUSBMIDIGadget::AddEndpoints (void)
{
	assert (!m_pEP[EPOut]);
	m_pEP[EPOut] = new CUSBMIDIGadgetEndpoint (
				reinterpret_cast<const TUSBEndpointDescriptor *> (
					&s_ConfigurationDescriptor.EndpointOut), this);
	assert (m_pEP[EPOut]);

	assert (!m_pEP[EPIn]);
	m_pEP[EPIn] = new CUSBMIDIGadgetEndpoint (
				reinterpret_cast<const TUSBEndpointDescriptor *> (
					&s_ConfigurationDescriptor.EndpointIn), this);
	assert (m_pEP[EPIn]);
}
