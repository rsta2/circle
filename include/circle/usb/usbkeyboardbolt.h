//
// usbkeyboardbolt.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2026  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbkeyboardbolt_h
#define _circle_usb_usbkeyboardbolt_h

#include <circle/usb/usbkeyboard.h>

class CUSBKeyboardBoltDevice : public CUSBKeyboardDevice
{
public:
	CUSBKeyboardBoltDevice (CUSBFunction *pFunction);
	boolean Configure (void);

protected:
	u16 GetHIDProtocol (void) const override;
	void ReportHandler (const u8 *pReport, unsigned nReportSize);
};

#endif
