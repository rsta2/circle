// SPDX-License-Identifier: GPL-3.0-or-later
//
// usb.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
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
#pragma once
#include <circle/types.h>
struct TSetupData
{
	u8 bmRequestType, bRequest;
	u16 wValue, wIndex, wLength;
};

#define DESCRIPTOR_DEVICE 1
#define DESCRIPTOR_CONFIGURATION 2
#define DESCRIPTOR_STRING 3
#define DESCRIPTOR_ENDPOINT 5
struct __attribute__((packed)) TUSBDeviceDescriptor
{
 u8 bLength, bDescriptorType;
 u16 bcdUSB;
 u8 bDeviceClass, bDeviceSubClass, bDeviceProtocol, bMaxPacketSize0;
 u16 idVendor, idProduct, bcdDevice;
 u8 iManufacturer, iProduct, iSerialNumber, bNumConfigurations;
};
struct __attribute__((packed)) TUSBEndpointDescriptor
{
 u8 bLength, bDescriptorType, bEndpointAddress, bmAttributes;
 u16 wMaxPacketSize;
 u8 bInterval;
};

#define DESCRIPTOR_CS_INTERFACE 36
