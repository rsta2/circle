//
// usbaudio.h
//
// Ported from the USPi driver which is:
// 	Copyright (C) 2016  J. Otto <joshua.t.otto@gmail.com>
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2023  R. Stange <rsta2@o2online.de>
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
#define USB_MIDI_STREAMING_SUBTYPE_GENERAL		0x01
	unsigned char	bNumEmbMIDIJack;
	unsigned char	bAssocJackIDs[1];
}
PACKED;

struct TUSBMIDIStreamingInterfaceDescriptorHeader
{
	unsigned char	bLength;
	unsigned char	bDescriptorType;
	unsigned char	bDescriptorSubtype;
#define USB_MIDI_STREAMING_IFACE_SUBTYPE_HEADER		0x01
	unsigned short	bcdADC;
#define USB_MIDI_STREAMING_IFACE_BCDADC_100		0x100
	unsigned short	wTotalLength;
}
PACKED;

struct TUSBMIDIStreamingInterfaceDescriptorInJack
{
	unsigned char	bLength;
	unsigned char	bDescriptorType;
	unsigned char	bDescriptorSubtype;
#define USB_MIDI_STREAMING_IFACE_SUBTYPE_MIDI_IN_JACK	0x02
	unsigned char	bJackType;
#define USB_MIDI_STREAMING_IFACE_JACKTYPE_EMBEDDED	0x01
#define USB_MIDI_STREAMING_IFACE_JACKTYPE_EXTERNAL	0x02
	unsigned char	bJackID;
	unsigned char	iJack;
}
PACKED;

struct TUSBMIDIStreamingInterfaceDescriptorOutJack
{
	unsigned char	bLength;
	unsigned char	bDescriptorType;
	unsigned char	bDescriptorSubtype;
#define USB_MIDI_STREAMING_IFACE_SUBTYPE_MIDI_OUT_JACK	0x03
	unsigned char	bJackType;
	unsigned char	bJackID;
	unsigned char	bNrInputPins;
	unsigned char	baSourceID[1];
	unsigned char	baSourcePin[1];
	unsigned char	iJack;
}
PACKED;

struct TUSBAudioControlInterfaceDescriptorHeader
{
	unsigned char	bLength;
	unsigned char	bDescriptorType;
	unsigned char	bDescriptorSubtype;
	unsigned short	bcdADC;
	unsigned short	wTotalLength;
	unsigned char	bInCollection;
	unsigned char	baInterfaceNr[1];
}
PACKED;

struct TUSBAudioControlInterfaceDescriptor
{
	unsigned char	bLength;
	unsigned char	bDescriptorType;
	unsigned char	bDescriptorSubtype;
#define USB_AUDIO_CTL_IFACE_SUBTYPE_HEADER		0x01
#define USB_AUDIO_CTL_IFACE_SUBTYPE_INPUT_TERMINAL	0x02
#define USB_AUDIO_CTL_IFACE_SUBTYPE_OUTPUT_TERMINAL	0x03
#define USB_AUDIO_CTL_IFACE_SUBTYPE_MIXER_UNIT		0x04
#define USB_AUDIO_CTL_IFACE_SUBTYPE_SELECTOR_UNIT	0x05
#define USB_AUDIO_CTL_IFACE_SUBTYPE_FEATURE_UNIT	0x06
#define USB_AUDIO_CTL_IFACE_SUBTYPE_CLOCK_SOURCE	0x0A		// v2.00 only
#define USB_AUDIO_CTL_IFACE_SUBTYPE_CLOCK_SELECTOR	0x0B		// v2.00 only

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

			struct
			{
				unsigned char	bTerminalID;
				unsigned short	wTerminalType	PACKED;
#define USB_AUDIO_TERMINAL_TYPE_USB_UNDEFINED		0x100
#define USB_AUDIO_TERMINAL_TYPE_USB_STREAMING		0x101
#define USB_AUDIO_TERMINAL_TYPE_SPEAKER			0x301
#define USB_AUDIO_TERMINAL_TYPE_SPDIF			0x605
				unsigned char	bAssocTerminal;
				unsigned char	bNrChannels;
				unsigned short	wChannelConfig	PACKED;
				unsigned char	iChannelNames;
				unsigned char	iTerminal;
			}
			InputTerminal;

			struct
			{
				unsigned char	bTerminalID;
				unsigned short	wTerminalType	PACKED;
				unsigned char	bAssocTerminal;
				unsigned char	bSourceID;
				unsigned char	iTerminal;
			}
			OutputTerminal;

			struct
			{
				unsigned char	bUnitID;
				unsigned char	bNrInPins;
				unsigned char	baSourceID[];
			}
			MixerUnit;

			struct
			{
				unsigned char	bUnitID;
				unsigned char	bNrInPins;
				unsigned char	baSourceID[];
				//unsigned char	iSelector;
			}
			SelectorUnit;

			struct
			{
				unsigned char	bUnitID;
				unsigned char	bSourceID;
				unsigned char	bControlSize;
				unsigned char	bmaControls[];
				//unsigned char	iFeature;
			}
			FeatureUnit;
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

			struct
			{
				unsigned char	bTerminalID;
				unsigned short	wTerminalType	PACKED;
				unsigned char	bAssocTerminal;
				unsigned char	bSourceID;
				unsigned char	bCSourceID;
				unsigned short	bmControls	PACKED;
				unsigned char	iTerminal;
			}
			OutputTerminal;

			struct
			{
				unsigned char	bUnitID;
				unsigned char	bNrInPins;
				unsigned char	baSourceID[];
			}
			MixerUnit;

			struct
			{
				unsigned char	bUnitID;
				unsigned char	bNrInPins;
				unsigned char	baSourceID[];
				//unsigned char	bmControls;
				//unsigned char	iSelector;
			}
			SelectorUnit;

			struct
			{
				unsigned char	bUnitID;
				unsigned char	bSourceID;
				unsigned int	bmaControls[]	PACKED;
				//unsigned char	iFeature;
			}
			FeatureUnit;

			struct
			{
				unsigned char	bClockID;
				unsigned char	bmAttributes;
				unsigned char	bmControls;
				unsigned char	bAssocTerminal;
				unsigned char	iClockSource;
			}
			ClockSource;

			struct
			{
				unsigned char	bClockID;
				unsigned char	bNrInPins;
				unsigned char	baCSourceID[];
				//unsigned char	bmControls;
				//unsigned char	iClockSelector;
			}
			ClockSelector;
		}
		Ver200;
	};
}
PACKED;

struct TUSBAudioControlMixerUnitTrailerVer100
{
	unsigned char	bNrChannels;
	unsigned short	wChannelConfig	PACKED;
	unsigned char	iChannelNames;
	unsigned char	bmControls[];
	//unsigned char	iMixer;
}
PACKED;

struct TUSBAudioControlMixerUnitTrailerVer200
{
	unsigned char	bNrChannels;
	unsigned int	bmChannelConfig	PACKED;
	unsigned char	iChannelNames;
	unsigned char	bmMixerControls[];
	//unsigned char	bmControls;
	//unsigned char	iMixer;
}
PACKED;

struct TUSBAudioStreamingInterfaceDescriptor
{
        unsigned char	bLength;
        unsigned char	bDescriptorType;
        unsigned char	bDescriptorSubtype;
#define USB_AUDIO_STREAMING_GENERAL		0x01

	union
	{
		struct
		{
			unsigned char	bTerminalLink;
			unsigned char	bDelay;
			unsigned short	wFormatTag	PACKED;
		}
		Ver100;

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
	};
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

#define USB_AUDIO_REQ_GET_MIN		0x82		// v1.00 only
#define USB_AUDIO_REQ_GET_MAX		0x83		// v1.00 only

// Audio class control selectors
#define USB_AUDIO_CS_SAM_FREQ_CONTROL	0x01

#define USB_AUDIO_FU_MUTE_CONTROL	0x01
#define USB_AUDIO_FU_VOLUME_CONTROL	0x02

#define USB_AUDIO_SU_SELECTOR_CONTROL	0x01		// v2.00 only

#define USB_AUDIO_CX_SELECTOR_CONTROL	0x01		// v2.00 only

#endif
