//
// usbgamepad.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbgamepad.h>
#include <circle/usb/usbhid.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/devicenameservice.h>
#include <circle/logger.h>
#include <circle/util.h>
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

unsigned CUSBGamePadDevice::s_nDeviceNumber = 1;

static const char FromUSBPad[] = "usbpad";

CUSBGamePadDevice::CUSBGamePadDevice (CUSBFunction *pFunction)
:	CUSBHIDDevice (pFunction),
	m_pStatusHandler (0),
	m_pHIDReportDescriptor (0),
	m_usReportDescriptorLength (0),
	m_usReportSize (0)
{
	m_State.naxes = 0;
	for (int i = 0; i < MAX_AXIS; i++)
	{
		m_State.axes[i].value = 0;
		m_State.axes[i].minimum = 0;
		m_State.axes[i].maximum = 0;
	}

	m_State.nhats = 0;
	for (int i = 0; i < MAX_HATS; i++)
	{
		m_State.hats[i] = 0;
	}

	m_State.nbuttons = 0;
	m_State.buttons = 0;
}

CUSBGamePadDevice::~CUSBGamePadDevice (void)
{
	m_pStatusHandler = 0;

	delete [] m_pHIDReportDescriptor;
	m_pHIDReportDescriptor = 0;
}

boolean CUSBGamePadDevice::Configure (void)
{
	TUSBHIDDescriptor *pHIDDesc = (TUSBHIDDescriptor *) GetDescriptor (DESCRIPTOR_HID);
	if (   pHIDDesc == 0
	    || pHIDDesc->wReportDescriptorLength == 0)
	{
		ConfigurationError (FromUSBPad);

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
		CLogger::Get ()->Write (FromUSBPad, LogError, "Cannot get HID report descriptor");

		return FALSE;
	}
	//debug_hexdump (m_pHIDReportDescriptor, m_usReportDescriptorLength, FromUSBPad);

	u8 ReportBuffer[8] = {0};
	DecodeReport (ReportBuffer);

	// ignoring unsupported HID interface
	if (   m_State.naxes    == 0
	    && m_State.nhats    == 0
	    && m_State.nbuttons == 0)
	{
		return FALSE;
	}

	assert (m_usReportSize != 0);
	if (!CUSBHIDDevice::Configure (m_usReportSize))
	{
		CLogger::Get ()->Write (FromUSBPad, LogError, "Cannot configure HID device");

		return FALSE;
	}

	const TUSBDeviceDescriptor *pDeviceDesc = GetDevice ()->GetDeviceDescriptor ();
	assert (pDeviceDesc != 0);
	if (   pDeviceDesc->idVendor  == 0x054C
	    && pDeviceDesc->idProduct == 0x0268)
	{
		PS3Configure ();
	}

	m_nDeviceNumber = s_nDeviceNumber++;

	CString DeviceName;
	DeviceName.Format ("upad%u", m_nDeviceNumber);
	CDeviceNameService::Get ()->AddDevice (DeviceName, this, FALSE);

	return TRUE;
}

const TGamePadState *CUSBGamePadDevice::GetReport (void)
{
	assert (0 < m_usReportSize && m_usReportSize < 64);
	u8 ReportBuffer[m_usReportSize];
	if (GetHost ()->ControlMessage (GetEndpoint0 (),
					REQUEST_IN | REQUEST_CLASS | REQUEST_TO_INTERFACE,
					GET_REPORT, (REPORT_TYPE_INPUT << 8) | 0x00,
					GetInterfaceNumber (),
					ReportBuffer, m_usReportSize) <= 0)
	{
		return 0;
	}

	DecodeReport (ReportBuffer);

	return &m_State;
}

void CUSBGamePadDevice::RegisterStatusHandler (TGamePadStatusHandler *pStatusHandler)
{
	assert (m_pStatusHandler == 0);
	m_pStatusHandler = pStatusHandler;
	assert (m_pStatusHandler != 0);
}

void CUSBGamePadDevice::ReportHandler (const u8 *pReport)
{
	if (   pReport != 0
	    && m_pStatusHandler != 0)
	{
		//debug_hexdump (pReport, m_usReportSize, FromUSBPad);

		DecodeReport (pReport);

		(*m_pStatusHandler) (m_nDeviceNumber-1, &m_State);
	}
}

void CUSBGamePadDevice::DecodeReport (const u8 *pReportBuffer)
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

u32 CUSBGamePadDevice::BitGetUnsigned (const void *buffer, u32 offset, u32 length)
{
	u8 *bitBuffer = (u8 *) buffer;
	u32 result = 0;
	u8 mask;

	for (u32 i = offset / 8, j = 0; i < (offset + length + 7) / 8; i++)
	{
		if (offset / 8 == (offset + length - 1) / 8)
		{
			mask = (1 << ((offset % 8) + length)) - (1 << (offset % 8));
			result = (bitBuffer[i] & mask) >> (offset % 8);
		}
		else if (i == offset / 8)
		{
			mask = 0x100 - (1 << (offset % 8));
			j += 8 - (offset % 8);
			result = ((bitBuffer[i] & mask) >> (offset % 8)) << (length - j);
		}
		else if (i == (offset + length - 1) / 8)
		{
			mask = (1 << ((offset % 8) + length)) - 1;
			result |= bitBuffer[i] & mask;
		}
		else
		{
			j += 8;
			result |= bitBuffer[i] << (length - j);
		}
	}

	return result;
}

s32 CUSBGamePadDevice::BitGetSigned (const void *buffer, u32 offset, u32 length)
{
	u32 result = BitGetUnsigned(buffer, offset, length);

	if (result & (1 << (length - 1)))
	{
		result |= 0xffffffff - ((1 << length) - 1);
	}

	return result;
}

void CUSBGamePadDevice::PS3Configure (void)
{
	static u8 writeBuf[] =
	{
		0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0xFF, 0x27, 0x10, 0x00, 0x32,
		0xFF, 0x27, 0x10, 0x00, 0x32,
		0xFF, 0x27, 0x10, 0x00, 0x32,
		0xFF, 0x27, 0x10, 0x00, 0x32,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00
	};

	static const u8 leds[] =
	{
		0x00, // OFF
		0x01, // LED1
		0x02, // LED2
		0x04, // LED3
		0x08  // LED4
	};

	/* Special PS3 Controller enable commands */
	static u8 Enable[] = {0x42, 0x0C, 0x00, 0x00};
	GetHost ()->ControlMessage (GetEndpoint0 (),
				    REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
				    SET_REPORT, (REPORT_TYPE_FEATURE << 8) | 0xF4,
				    GetInterfaceNumber (), Enable, sizeof Enable);

	/* Turn on LED */
	writeBuf[9] = (u8)(leds[m_nDeviceNumber] << 1);
	GetHost ()->ControlMessage (GetEndpoint0 (),
				    REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
				    SET_REPORT, (REPORT_TYPE_OUTPUT << 8) | 0x01,
				    GetInterfaceNumber (), writeBuf, sizeof writeBuf);
}
