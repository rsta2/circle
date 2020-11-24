//
// usbhid.h
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
#ifndef _circle_usb_usbhid_h
#define _circle_usb_usbhid_h

#include <circle/macros.h>

// Class-specific requests
#define GET_REPORT		0x01
#define SET_REPORT		0x09
#define SET_PROTOCOL		0x0B

// Class-specific descriptors
#define DESCRIPTOR_HID		0x21
#define DESCRIPTOR_REPORT	0x22

// Protocol IDs
#define BOOT_PROTOCOL		0x00
#define REPORT_PROTOCOL		0x01

// Report types
#define REPORT_TYPE_INPUT	0x01
#define REPORT_TYPE_OUTPUT	0x02
#define REPORT_TYPE_FEATURE	0x03

struct TUSBHIDDescriptor
{
	unsigned char	bLength;
	unsigned char	bDescriptorType;
	unsigned short	bcdHID;
	unsigned char	bCountryCode;
	unsigned char	bNumDescriptors;
	unsigned char	bReportDescriptorType;
	unsigned short	wReportDescriptorLength;
}
PACKED;

// Modifiers (boot protocol)
#define LCTRL		(1 << 0)
#define LSHIFT		(1 << 1)
#define ALT		(1 << 2)
#define LWIN		(1 << 3)
#define RCTRL		(1 << 4)
#define RSHIFT		(1 << 5)
#define ALTGR		(1 << 6)
#define RWIN		(1 << 7)

// LEDs (boot protocol)
#define LED_NUM_LOCK	(1 << 0)
#define LED_CAPS_LOCK	(1 << 1)
#define LED_SCROLL_LOCK	(1 << 2)

// Mouse buttons (boot protocol)
#define USBHID_BUTTON1	(1 << 0)
#define USBHID_BUTTON2	(1 << 1)
#define USBHID_BUTTON3	(1 << 2)
#define USBHID_BUTTON4	(1 << 3)
#define USBHID_BUTTON5	(1 << 4)

#endif
