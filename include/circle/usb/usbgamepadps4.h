//
// usbgamepadps4.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2018  R. Stange <rsta2@o2online.de>
//
// This driver was developed by:
//	Jose Luis Sanchez, http://jspeccy.speccy.org/
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
#ifndef _circle_usb_usbgamepadps4_h
#define _circle_usb_usbgamepadps4_h

#include <circle/usb/usbgamepad.h>
#include <circle/input/mouse.h>
#include <circle/types.h>

struct PS4Output {
    u8 rumbleState, bigRumble, smallRumble; // Rumble
    u8 red, green, blue; // RGB
    u8 flashOn, flashOff; // Time to flash bright/dark (255 = 2.5 seconds)
};

class CUSBGamePadPS4Device : public CUSBGamePadDevice
{
public:
	CUSBGamePadPS4Device (CUSBFunction *pFunction);
	~CUSBGamePadPS4Device (void);

	boolean Configure (void);

	unsigned GetProperties (void)
	{
		return   GamePadPropertyIsKnown
		       | GamePadPropertyHasLED
		       | GamePadPropertyHasRGBLED
		       | GamePadPropertyHasRumble
		       | GamePadPropertyHasGyroscope
		       | GamePadPropertyHasTouchpad;
	}

	boolean SetLEDMode (TGamePadLEDMode Mode);
	boolean SetLEDMode (u32 nRGB, u8 uchTimeOn, u8 uchTimeOff);
	boolean SetRumbleMode (TGamePadRumbleMode Mode);

	static void DisableTouchpad (void);

private:
	void ReportHandler (const u8 *pReport, unsigned nReportSize);
	void DecodeReport (const u8 *pReportBuffer);
	void HandleTouchpad (const u8 *pReportBuffer);
	boolean SendLedRumbleCommand(void);

private:
	boolean m_bInterfaceOK;
	PS4Output ps4Output;
	u8 *outBuffer;

	CMouseDevice *m_pMouseDevice;
	struct
	{
		boolean bButtonPressed;
		boolean	bTouched;
		u16	usPosX;
		u16	usPosY;
	}
	m_Touchpad;		// previous state from touchpad

	static boolean s_bTouchpadEnabled;
};

#endif
