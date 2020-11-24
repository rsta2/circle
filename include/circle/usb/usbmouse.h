//
// usbmouse.h
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
#ifndef _circle_usb_usbmouse_h
#define _circle_usb_usbmouse_h

#include <circle/usb/usbhiddevice.h>
#include <circle/input/mouse.h>
#include <circle/types.h>

enum TMouseReportType
{
	MouseItemButtons,
	MouseItemXAxis,
	MouseItemYAxis,
	MouseItemWheel,

	MouseItemCount
};

struct TMouseReportItem
{
	unsigned bitSize;
	unsigned bitOffset;
};

struct TMouseReport
{
	unsigned id;
	unsigned byteSize;
	TMouseReportItem items[MouseItemCount];
};

class CUSBMouseDevice : public CUSBHIDDevice
{
public:
	CUSBMouseDevice (CUSBFunction *pFunction);
	~CUSBMouseDevice (void);

	boolean Configure (void);

private:
	void ReportHandler (const u8 *pReport, unsigned nReportSize);
	void DecodeReport (void);
	u32 ExtractUnsigned (const void *buffer, u32 offset, u32 length);
	s32 ExtractSigned (const void *buffer, u32 offset, u32 length);

private:
	CMouseDevice *m_pMouseDevice;

	u8 *m_pHIDReportDescriptor;
	u16 m_usReportDescriptorLength;

	TMouseReport m_MouseReport;
};

#endif
