//
// usbgamepadstandard.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2018  R. Stange <rsta2@o2online.de>
//
// Ported from the USPi driver which is:
// 	Copyright (C) 2014  M. Maccaferri <macca@maccasoft.com>
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
#include <circle/usb/usbgamepadstandard.h>
#include <circle/usb/usbhid.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/logger.h>
#include <circle/debug.h>
#include <assert.h>

// HID Report Items from HID 1.11 Section 6.2.2
#define HID_USAGE_PAGE      0x04
#define HID_USAGE           0x08
#define HID_COLLECTION      0xA0
#define HID_END_COLLECTION  0xC0
#define HID_REPORT_COUNT    0x94
#define HID_REPORT_SIZE     0x74
#define HID_USAGE_MIN       0x18
#define HID_USAGE_MAX       0x28
#define HID_LOGICAL_MIN     0x14
#define HID_LOGICAL_MAX     0x24
#define HID_PHYSICAL_MIN    0x34
#define HID_PHYSICAL_MAX    0x44
#define HID_INPUT           0x80
#define HID_REPORT_ID       0x84
#define HID_OUTPUT          0x90

// HID Report Usage Pages from HID Usage Tables 1.12 Section 3, Table 1
#define HID_USAGE_PAGE_GENERIC_DESKTOP 0x01
#define HID_USAGE_PAGE_KEY_CODES       0x07
#define HID_USAGE_PAGE_LEDS            0x08
#define HID_USAGE_PAGE_BUTTONS         0x09

// HID Report Usages from HID Usage Tables 1.12 Section 4, Table 6
#define HID_USAGE_POINTER   0x01
#define HID_USAGE_MOUSE     0x02
#define HID_USAGE_JOYSTICK  0x04
#define HID_USAGE_GAMEPAD   0x05
#define HID_USAGE_KEYBOARD  0x06
#define HID_USAGE_X         0x30
#define HID_USAGE_Y         0x31
#define HID_USAGE_Z         0x32
#define HID_USAGE_RX        0x33
#define HID_USAGE_RY        0x34
#define HID_USAGE_RZ        0x35
#define HID_USAGE_SLIDER    0x36
#define HID_USAGE_WHEEL     0x38
#define HID_USAGE_HATSWITCH 0x39

// HID Report Collection Types from HID 1.12 6.2.2.6
#define HID_COLLECTION_PHYSICAL    0
#define HID_COLLECTION_APPLICATION 1

// HID Input/Output/Feature Item Data (attributes) from HID 1.11 6.2.2.5
#define HID_ITEM_CONSTANT 0x1
#define HID_ITEM_VARIABLE 0x2
#define HID_ITEM_RELATIVE 0x4

static const char FromUSBPadStd[] = "usbpadstd";

CUSBGamePadStandardDevice::CUSBGamePadStandardDevice (CUSBFunction *pFunction,
						      boolean bAutoStartRequest)
:	CUSBGamePadDevice (pFunction),
	m_bAutoStartRequest (bAutoStartRequest),
	m_pHIDReportDescriptor (0),
	m_usReportDescriptorLength (0)
{
}

CUSBGamePadStandardDevice::~CUSBGamePadStandardDevice (void)
{
	delete [] m_pHIDReportDescriptor;
	m_pHIDReportDescriptor = 0;
}

boolean CUSBGamePadStandardDevice::Configure (void)
{
	TUSBHIDDescriptor *pHIDDesc = (TUSBHIDDescriptor *) GetDescriptor (DESCRIPTOR_HID);
	if (   pHIDDesc == 0
	    || pHIDDesc->wReportDescriptorLength == 0)
	{
		ConfigurationError (FromUSBPadStd);

		return FALSE;
	}

	m_usReportDescriptorLength = pHIDDesc->wReportDescriptorLength;
	m_pHIDReportDescriptor = new u8[m_usReportDescriptorLength];
	assert (m_pHIDReportDescriptor != 0);

	if (   GetHost ()->GetDescriptor (GetEndpoint0 (),
					  pHIDDesc->bReportDescriptorType, DESCRIPTOR_INDEX_DEFAULT,
					  m_pHIDReportDescriptor, m_usReportDescriptorLength,
					  REQUEST_IN | REQUEST_TO_INTERFACE, GetInterfaceNumber ())
	    != m_usReportDescriptorLength)
	{
		CLogger::Get ()->Write (FromUSBPadStd, LogError, "Cannot get HID report descriptor");

		return FALSE;
	}
	//debug_hexdump (m_pHIDReportDescriptor, m_usReportDescriptorLength, FromUSBPadStd);

	u8 ReportBuffer[100] = {0};
	DecodeReport (ReportBuffer);

	// ignoring unsupported HID interface
	if (   m_State.naxes    == 0
	    && m_State.nhats    == 0
	    && m_State.nbuttons == 0)
	{
		return FALSE;
	}

	assert (m_usReportSize != 0);
	if (!CUSBGamePadDevice::Configure ())
	{
		CLogger::Get ()->Write (FromUSBPadStd, LogError, "Cannot configure gamepad device");

		return FALSE;
	}

	return m_bAutoStartRequest ? StartRequest () : TRUE;
}

void CUSBGamePadStandardDevice::DecodeReport (const u8 *pReportBuffer)
{
	s32 item, arg;
	u32 offset = 0, size = 0, count = 0;
#define UNDEFINED	-123456789
	s32 lmax = UNDEFINED, lmin = UNDEFINED, pmin = UNDEFINED, pmax = UNDEFINED;
	s32 naxes = 0, nhats = 0;
	u32 id = 0;
	enum
	{
		None,
		GamePad,
		GamePadButton,
		GamePadAxis,
		GamePadHat
	}
	state = None;

	assert (m_pHIDReportDescriptor != 0);
	s8 *pHIDReportDescriptor = (s8 *) m_pHIDReportDescriptor;

	for (u16 usReportDescriptorLength = m_usReportDescriptorLength; usReportDescriptorLength > 0; )
	{
		item = *pHIDReportDescriptor++;
		usReportDescriptorLength--;

		switch(item & 0x03)
		{
		case 0:
			arg = 0;
			break;
		case 1:
			arg = *pHIDReportDescriptor++;
			usReportDescriptorLength--;
			break;
		case 2:
			arg = *pHIDReportDescriptor++ & 0xFF;
			arg = arg | (*pHIDReportDescriptor++ << 8);
			usReportDescriptorLength -= 2;
			break;
		default:
			arg = *pHIDReportDescriptor++;
			arg = arg | (*pHIDReportDescriptor++ << 8);
			arg = arg | (*pHIDReportDescriptor++ << 16);
			arg = arg | (*pHIDReportDescriptor++ << 24);
			usReportDescriptorLength -= 4;
			break;
		}

		if ((item & 0xFC) == HID_REPORT_ID)
		{
			if (id != 0)
				break;
			id = BitGetUnsigned(pReportBuffer, 0, 8);
			if (id != 0 && (int) id != arg)
				return;
			id = arg;
			offset = 8;
		}

		switch(item & 0xFC)
		{
		case HID_USAGE_PAGE:
			switch(arg)
			{
			case HID_USAGE_PAGE_BUTTONS:
				if (state == GamePad)
					state = GamePadButton;
				break;
			}
			break;
		case HID_USAGE:
			switch(arg)
			{
			case HID_USAGE_JOYSTICK:
			case HID_USAGE_GAMEPAD:
				state = GamePad;
				break;
			case HID_USAGE_X:
			case HID_USAGE_Y:
			case HID_USAGE_Z:
			case HID_USAGE_RX:
			case HID_USAGE_RY:
			case HID_USAGE_RZ:
			case HID_USAGE_SLIDER:
				if (state == GamePad)
					state = GamePadAxis;
				break;
			case HID_USAGE_HATSWITCH:
				if (state == GamePad)
					state = GamePadHat;
				break;
			}
			break;
		case HID_LOGICAL_MIN:
			lmin = arg;
			break;
		case HID_PHYSICAL_MIN:
			pmin = arg;
			break;
		case HID_LOGICAL_MAX:
			lmax = arg;
			break;
		case HID_PHYSICAL_MAX:
			pmax = arg;
			break;
		case HID_REPORT_SIZE: // REPORT_SIZE
			size = arg;
			break;
		case HID_REPORT_COUNT: // REPORT_COUNT
			count = arg;
			break;
		case HID_INPUT:
			if ((arg & 0x03) == 0x02)	// INPUT(Data,Var)
			{
				if (state == GamePadAxis)
				{
					for (unsigned i = 0; i < count && i < MAX_AXIS; i++)
					{
						m_State.axes[naxes].minimum = lmin != UNDEFINED ? lmin : pmin;
						m_State.axes[naxes].maximum = lmax != UNDEFINED ? lmax : pmax;

						int value = (m_State.axes[naxes].minimum < 0) ?
							BitGetSigned(pReportBuffer, offset + i * size, size) :
							BitGetUnsigned(pReportBuffer, offset + i * size, size);

						m_State.axes[naxes++].value = value;
					}

					state = GamePad;
				}
				else if (state == GamePadHat)
				{
					for (unsigned i = 0; i < count && i < MAX_HATS; i++)
					{
						int value = BitGetUnsigned(pReportBuffer, offset + i * size, size);
						m_State.hats[nhats++] = value;
					}
					state = GamePad;
				}
				else if (state == GamePadButton)
				{
					m_State.nbuttons = count;
					m_State.buttons = BitGetUnsigned(pReportBuffer, offset, size * count);
					state = GamePad;
				}
			}
			offset += count * size;
			break;
		case HID_OUTPUT:
			break;
		}
	}

	m_State.naxes = naxes;
	m_State.nhats = nhats;

	m_usReportSize = (offset + 7) / 8;
}

u32 CUSBGamePadStandardDevice::BitGetUnsigned (const void *buffer, u32 offset, u32 length)
{
	assert(buffer != 0);
	assert(length <= 32);

	if (length == 0)
	{
		return 0;
	}

	u8 *bits = (u8 *)buffer;
	unsigned shift = offset % 8;
	offset = offset / 8;
	bits = bits + offset;
	unsigned number = *(unsigned *)bits;
	offset = shift;

	unsigned result = 0;
	if (length > 24)
	{
		result = (((1 << 24) - 1) & (number >> offset));
		bits = bits + 3;
		number = *(unsigned *)bits;
		length = length - 24;
		unsigned result2 = (((1 << length) - 1) & (number >> offset));
		result = (result2 << 24) | result;
	}
	else
	{
		result = (((1 << length) - 1) & (number >> offset));
	}

	return result;
}

s32 CUSBGamePadStandardDevice::BitGetSigned (const void *buffer, u32 offset, u32 length)
{
	assert(buffer != 0);
	assert(length <= 32);

	unsigned result = BitGetUnsigned(buffer, offset, length);
	if ((length == 0) || (length == 32))
	{
		return result;
	}

	if (result & (1 << (length - 1)))
	{
		result |= 0xffffffff - ((1 << length) - 1);
	}

	return result;
}
