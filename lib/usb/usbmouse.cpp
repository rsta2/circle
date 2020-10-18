//
// usbmouse.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2018  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbmouse.h>
#include <circle/usb/usbhid.h>
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

// in boot protocol 3 bytes are received, in report protocol 5 bytes are received
//#define REPORT_SIZE 3
//#define REPORT_SIZE 5

static const char FromUSBMouse[] = "umouse";

CUSBMouseDevice::CUSBMouseDevice (CUSBFunction *pFunction)
:	CUSBHIDDevice (pFunction),
	m_pMouseDevice (0),
	m_pHIDReportDescriptor (0)
{
}

CUSBMouseDevice::~CUSBMouseDevice (void)
{
	delete m_pMouseDevice;
	m_pMouseDevice = 0;

	delete [] m_pHIDReportDescriptor;
	m_pHIDReportDescriptor = 0;
}

boolean CUSBMouseDevice::Configure (void)
{
	TUSBHIDDescriptor *pHIDDesc = (TUSBHIDDescriptor *) GetDescriptor (DESCRIPTOR_HID);
	if (   pHIDDesc == 0
	    || pHIDDesc->wReportDescriptorLength == 0)
	{
		ConfigurationError (FromUSBMouse);

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
		CLogger::Get ()->Write (FromUSBMouse, LogError, "Cannot get HID report descriptor");

		return FALSE;
	}
    CLogger::Get ()->Write (FromUSBMouse, LogDebug, "Report descriptor");
    debug_hexdump (m_pHIDReportDescriptor, m_usReportDescriptorLength, FromUSBMouse);

	DecodeReport ();

	// ignoring unsupported HID interface
	if (m_ReportItems.nItems == 0)
	{
		return FALSE;
	}

	if (!CUSBHIDDevice::Configure (m_ReportItems.size))
	{
		CLogger::Get ()->Write (FromUSBMouse, LogError, "Cannot configure HID device");

		return FALSE;
	}

	m_pMouseDevice = new CMouseDevice;
	assert (m_pMouseDevice != 0);

	return StartRequest ();
}

void CUSBMouseDevice::ReportHandler (const u8 *pReport, unsigned nReportSize)
{
//    debug_hexdump (pReport, nReportSize, FromUSBMouse);
    if (   pReport != 0
	    && nReportSize == m_ReportItems.size)
	{
        // in boot protocol the 3 bytes are:
        // 0   1 - left button, 2 - right button, 4 - middle button
        // 1   X displacement (+/- 127 max)
        // 2   Y displacement (+/- 127 max)
        // in report protocol the 5 bytes are:
        // 0   report ID (from report descriptor)
        // 1   1 - left button, 2 - right button, 4 - middle button
        // 2   X displacement (+/- 127 max)
        // 3   Y displacement (+/- 127 max)
        // 4   1 - wheel up, -1 wheel down

        if (m_pMouseDevice != 0)
        {
            u32 ucHIDButtons = 0;
            s32 xMove = 0;
            s32 yMove = 0;
            s32 wheelMove = 0;
            for (u32 index = 0; index < m_ReportItems.nItems; index++) {
                TMouseReportItem *item = &m_ReportItems.items[index];
                switch (item->type) {
                case MouseItemButtons:
                    ucHIDButtons = ExtractUnsigned(pReport, item->offset, item->count);
                    break;
                case MouseItemXAxis:
                    xMove = ExtractSigned(pReport, item->offset, item->count);
                    break;
                case MouseItemYAxis:
                    yMove = ExtractSigned(pReport, item->offset, item->count);
                    break;
                case MouseItemWheel:
                    wheelMove = ExtractSigned(pReport, item->offset, item->count);
                    break;
                }
            }

            u32 nButtons = 0;
            if (ucHIDButtons & USBHID_BUTTON1)
            {
                nButtons |= MOUSE_BUTTON_LEFT;
            }
            if (ucHIDButtons & USBHID_BUTTON2)
            {
                nButtons |= MOUSE_BUTTON_RIGHT;
            }
            if (ucHIDButtons & USBHID_BUTTON3)
            {
                nButtons |= MOUSE_BUTTON_MIDDLE;
            }

            //CLogger::Get ()->Write (FromUSBMouse, LogDebug, "2: %02X %2d %3d %3d %3d", pReport[0], nButtons, xMove, yMove, wheelMove);
            m_pMouseDevice->ReportHandler (nButtons, xMove, yMove, wheelMove);
		}
	}
}

u32 CUSBMouseDevice::ExtractUnsigned(const void *buffer, u32 offset, u32 length) {
    assert(buffer != 0);
    assert(length <= 32);

    u8 *bits = (u8 *)buffer;
    unsigned shift = offset % 8;
    offset = offset / 8;
    bits = bits + offset;
    unsigned number = *(unsigned *)bits;
    offset = shift;

    unsigned result = 0;
    if (length > 24) {
        result = (((1 << 24) - 1) & (number >> offset));
        bits = bits + 3;
        number = *(unsigned *)bits;
        length = length - 24;
        unsigned result2 = (((1 << length) - 1) & (number >> offset));
        result = (result2 << 24) | result;
    } else {
        result = (((1 << length) - 1) & (number >> offset));

    }
    return result;
}

s32 CUSBMouseDevice::ExtractSigned(const void *buffer, u32 offset, u32 length) {
    assert(buffer != 0);
    assert(length <= 32);

    unsigned result = ExtractUnsigned(buffer, offset, length);
    if (length == 32) {
        return result;
    }
    if (result & (1 << (length - 1))) {
		result |= 0xffffffff - ((1 << length) - 1);
	}
    return result;
}

void CUSBMouseDevice::DecodeReport ()
{
	s32 item, arg;
	u32 offset = 0, size = 0, count = 0;
	u32 id = 0;
    u32 nCollections = 0;
    u32 itemIndex = 0;
    u32 reportIndex = 0;
    boolean parse = FALSE;

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
        //fprintf(stderr, "item, arg %02X %04X\n", item, arg);

        switch(item & 0xFC)
		{
        case HID_COLLECTION:
            nCollections++;
            break;
        case HID_END_COLLECTION:
            nCollections--;
            if (nCollections == 0)
                parse = FALSE;
            break;
		case HID_USAGE_PAGE:
			switch(arg)
			{
            case HID_USAGE_PAGE_GENERIC_DESKTOP:
                //fprintf(stderr, "Generic Desktop Controls\n");
                break;
			}
			break;
		case HID_USAGE:
			switch(arg)
			{
            case HID_USAGE_MOUSE:
                parse = TRUE;
                //fprintf(stderr, "state: Mouse\n");
                break;
            }
            break;
        }

        if (! parse)
            continue;

		if ((item & 0xFC) == HID_REPORT_ID)
		{
            assert(id == 0);
            if (id != 0){
                break;
            }
			id = arg;
			offset = 8;
            //fprintf(stderr, "report ID %d\n", id);
		}

		switch(item & 0xFC)
		{
		case HID_USAGE_PAGE:
			switch(arg)
			{
			case HID_USAGE_PAGE_BUTTONS:
                //fprintf(stderr, "state: Mouse Buttons index %d\n", itemIndex);
                m_ReportItems.items[itemIndex].type = MouseItemButtons;
                itemIndex++;
				break;
			}
			break;
		case HID_USAGE:
			switch(arg)
			{
			case HID_USAGE_X:
                //fprintf(stderr, "state: Mouse X Axis index %d\n", itemIndex);
                m_ReportItems.items[itemIndex].type = MouseItemXAxis;
                itemIndex++;
                break;
			case HID_USAGE_Y:
                //fprintf(stderr, "state: Mouse Y Axis index %d\n", itemIndex);
                m_ReportItems.items[itemIndex].type = MouseItemYAxis;
                itemIndex++;
                break;
            case HID_USAGE_WHEEL:
                //fprintf(stderr, "state: Mouse Wheel index %d\n", itemIndex);
                m_ReportItems.items[itemIndex].type = MouseItemWheel;
                itemIndex++;
                break;
			}
			break;
		case HID_REPORT_SIZE:
			size = arg;
			break;
		case HID_REPORT_COUNT:
			count = arg;
			break;
		case HID_INPUT:
			if ((arg & 0x03) == 0x02)
			{
                //fprintf(stderr, "item index %d, report index %d\n", itemIndex, reportIndex);
                u32 tmp = offset;
                while (reportIndex < itemIndex)
                {
                    TMouseReportItem *item = &m_ReportItems.items[reportIndex];
                    switch (item->type)
                    {
                    case MouseItemButtons:
                        item->count = count * size;
                        item->offset = tmp;
                        break;
                    case MouseItemXAxis:
                    case MouseItemYAxis:
                    case MouseItemWheel:
                        item->count = size;
                        item->offset = tmp;
                        break;
                    }
                    tmp += item->count;
                    reportIndex++;
                }
            }
            offset += count * size;
			break;
		}
	}

    m_ReportItems.id = id;
    m_ReportItems.size = (offset + 7) / 8;
    m_ReportItems.nItems = itemIndex;
    CLogger::Get ()->Write (FromUSBMouse, LogDebug, "Report ID %d, size %d bytes, item count %d",
                            m_ReportItems.id, m_ReportItems.size, m_ReportItems.nItems);
    for (u32 i = 0; i < itemIndex; i++) {
        CLogger::Get ()->Write (FromUSBMouse, LogDebug, "[%d] item ID %d offset %d, count %d",
                                i, m_ReportItems.items[i].type, m_ReportItems.items[i].offset, m_ReportItems.items[i].count);
    }
}
