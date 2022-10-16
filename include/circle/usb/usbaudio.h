//
// usbaudio.h
//
// Ported from the USPi driver which is:
// 	Copyright (C) 2016  J. Otto <joshua.t.otto@gmail.com>
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2022  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbaudio_h
#define _circle_usb_usbaudio_h

#include <circle/macros.h>

// USB Audio class interface protocol numbers
#define USB_PROTO_AUDIO_VER_100		0x00
#define USB_PROTO_AUDIO_VER_200		0x20

// USB Audio class unit IDs
#define USB_AUDIO_UNDEFINED_UNIT_ID	0
#define USB_AUDIO_MAXIMUM_UNIT_ID	255

// Audio class endpoint descriptor (v1.00 only)
struct TUSBAudioEndpointDescriptor
{
	unsigned char	bLength;
	unsigned char	bDescriptorType;
	unsigned char	bEndpointAddress;
	unsigned char	bmAttributes;
	unsigned short	wMaxPacketSize;
	unsigned char	bInterval;
	unsigned char	bRefresh;
	unsigned char	bSynchAddress;
}
PACKED;

// MIDI-streaming class-specific endpoint descriptor (v1.00 only)
struct TUSBMIDIStreamingEndpointDescriptor
{
	unsigned char	bLength;
	unsigned char	bDescriptorType;
	unsigned char	bDescriptorSubType;
	unsigned char	bNumEmbMIDIJack;
	unsigned char	bAssocJackIDs[];
}
PACKED;

struct TUSBAudioControlInterfaceDescriptor
{
	unsigned char	bLength;
	unsigned char	bDescriptorType;
	unsigned char	bDescriptorSubtype;
#define USB_AUDIO_CTL_IFACE_SUBTYPE_HEADER		0x01
#define USB_AUDIO_CTL_IFACE_SUBTYPE_INPUT_TERMINAL	0x02

	union
	{
		union
		{
			struct
			{
				unsigned short	bcdADC		PACKED;
#define USB_AUDIO_CTL_IFACE_BCDADC_100			0x100
				unsigned short	wTotalLength	PACKED;
				unsigned char	bInCollection;
				unsigned char	baInterfaceNr[];
			}
			Header;
		}
		Ver100;

		union
		{
			struct
			{
				unsigned short	bcdADC		PACKED;
#define USB_AUDIO_CTL_IFACE_BCDADC_200			0x200
				unsigned char	bCategory;
				unsigned short	wTotalLength	PACKED;
				unsigned char	bmControls;
			}
			Header;

			struct
			{
				unsigned char	bTerminalID;
				unsigned short	wTerminalType	PACKED;
				unsigned char	bAssocTerminal;
				unsigned char	bCSourceID;
				unsigned char	bNrChannels;
				unsigned int	bmChannelConfig	PACKED;
				unsigned char	iChannelNames;
				unsigned short	bmControls	PACKED;
				unsigned char	iTerminal;
			}
			InputTerminal;
		}
		Ver200;
	};
}
PACKED;

struct CUSBAudioStreamingInterfaceDescriptor
{
        unsigned char	bLength;
        unsigned char	bDescriptorType;
        unsigned char	bDescriptorSubtype;
#define USB_AUDIO_STREAMING_GENERAL		0x01

	struct
	{
		unsigned char	bTerminalLink;
		unsigned char	bmControls;
		unsigned char	bFormatType;
		unsigned int	bmFormats	PACKED;
		unsigned char	bNrChannels;
		unsigned int	bmChannelConfig	PACKED;
		unsigned char	iChannelNames;
	}
	Ver200;
}
PACKED;

// Audio class type I format type descriptor
struct TUSBAudioTypeIFormatTypeDescriptor
{
	unsigned char	bLength;
	unsigned char	bDescriptorType;
	unsigned char	bDescriptorSubtype;
#define USB_AUDIO_FORMAT_TYPE			0x02
	unsigned char	bFormatType;
#define USB_AUDIO_FORMAT_TYPE_I			0x01

	union
	{
		struct
		{
			unsigned char	bNrChannels;
			unsigned char	bSubframeSize;
			unsigned char	bBitResolution;
			unsigned char	bSamFreqType;
			unsigned char	tSamFreq[][3];
		}
		Ver100;

		struct
		{
			unsigned char	bSubslotSize;
			unsigned char	bBitResolution;
		}
		Ver200;
	};
}
PACKED;

// Audio class control requests
#define USB_AUDIO_REQ_SET_CUR		0x01
#define USB_AUDIO_REQ_RANGE		0x02		// v2.00 only

// Audio class control selectors
#define USB_AUDIO_CS_SAM_FREQ_CONTROL	0x01

#endif
