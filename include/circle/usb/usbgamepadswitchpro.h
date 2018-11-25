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
#ifndef _circle_usb_usbgamepadswitchpro_h
#define _circle_usb_usbgamepadswitchpro_h

#include <circle/usb/usbgamepad.h>
#include <circle/types.h>

class CUSBGamePadSwitchProDevice : public CUSBGamePadDevice	/// Driver for Nintendo Switch Pro gamepad
{
public:
	CUSBGamePadSwitchProDevice (CUSBFunction *pFunction);
	~CUSBGamePadSwitchProDevice (void);

	boolean Configure (void);

	unsigned GetProperties (void)
	{
		return   GamePadPropertyIsKnown
		       | GamePadPropertyHasLED
		       | GamePadPropertyHasRumble
		       | GamePadPropertyHasAlternativeMapping;
	}

	boolean SetLEDMode (TGamePadLEDMode Mode);
	boolean SetRumbleMode (TGamePadRumbleMode);

private:
	void DecodeReport (const u8 *pReportBuffer);

private:
	boolean m_bInterfaceOK;
	u8 rumbleCounter;
};

#endif
