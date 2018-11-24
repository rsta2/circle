//
// usbgamepadstandard.h
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
#ifndef _circle_usb_usbgamepadstandard_h
#define _circle_usb_usbgamepadstandard_h

#include <circle/usb/usbgamepad.h>
#include <circle/types.h>

class CUSBGamePadStandardDevice : public CUSBGamePadDevice  /// Driver for HID class USB gamepads
{
public:
	CUSBGamePadStandardDevice (CUSBFunction *pFunction, boolean bAutoStartRequest = TRUE);
	~CUSBGamePadStandardDevice (void);

	boolean Configure (void);

protected:
	void DecodeReport (const u8 *pReportBuffer);

private:
	static u32 BitGetUnsigned (const void *buffer, u32 offset, u32 length);
	static s32 BitGetSigned (const void *buffer, u32 offset, u32 length);

private:
	boolean m_bAutoStartRequest;

	u8 *m_pHIDReportDescriptor;
	u16 m_usReportDescriptorLength;
};

#endif
