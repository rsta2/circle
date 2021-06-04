//
// usbtouchscreen.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbtouchscreen_h
#define _circle_usb_usbtouchscreen_h

#include <circle/usb/usbhiddevice.h>
#include <circle/input/touchscreen.h>
#include <circle/types.h>

class CUSBTouchScreenDevice : public CUSBHIDDevice	/// Driver for USB HID-class touchscreens
{
private:
	static const unsigned MaxContactCount = 10;

	struct TReportItem
	{
		unsigned	BitOffset;
		unsigned	BitSize;
	};

	struct TReportFinger
	{
		TReportItem	TipSwitch;
		TReportItem	ContactIdentifier;
		TReportItem	TipPressure;
		TReportItem	X;
		TReportItem	Y;
	};

	struct TReport
	{
		unsigned	ByteSize;
		u8		ReportID;
		unsigned	MaxContactCountActual;
		TReportItem	ContactCount;
		TReportFinger	Finger[MaxContactCount];
	};

public:
	CUSBTouchScreenDevice (CUSBFunction *pFunction);
	~CUSBTouchScreenDevice (void);

	boolean Configure (void);

private:
	void ReportHandler (const u8 *pReport, unsigned nReportSize);

	boolean DecodeReportDescriptor (const u8 *pDesc, unsigned nDescSize);

	static u32 GetValue (const void *pBuffer, const TReportItem &Item, u32 nDefault = 0);

private:
	TReport m_Report;

	boolean m_bFingerIsDown[MaxContactCount];
	u8 m_ucContactID[MaxContactCount];
	u16 m_usLastX[MaxContactCount];
	u16 m_usLastY[MaxContactCount];

	CTouchScreenDevice *m_pDevice;
};

#endif
