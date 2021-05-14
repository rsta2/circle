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

#define USBTOUCH_REPORT_SIZE	11

class CUSBTouchScreenDevice : public CUSBHIDDevice
{
public:
	CUSBTouchScreenDevice (CUSBFunction *pFunction);
	~CUSBTouchScreenDevice (void);

	boolean Configure (void);

private:
	void ReportHandler (const u8 *pReport, unsigned nReportSize);

private:
	boolean m_bFingerIsDown;
	u16 m_usLastX;
	u16 m_usLastY;

	CTouchScreenDevice *m_pDevice;
};

#endif
