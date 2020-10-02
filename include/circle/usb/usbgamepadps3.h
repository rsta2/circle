//
// usbgamepadps3.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbgamepadps3_h
#define _circle_usb_usbgamepadps3_h

#include <circle/usb/usbgamepadstandard.h>
#include <circle/synchronize.h>
#include <circle/macros.h>
#include <circle/types.h>

#define USB_GAMEPAD_PS3_COMMAND_LENGTH	48

class CUSBGamePadPS3Device : public CUSBGamePadStandardDevice
{
public:
	CUSBGamePadPS3Device (CUSBFunction *pFunction);
	~CUSBGamePadPS3Device (void);

	boolean Configure (void);

	unsigned GetProperties (void)
	{
		return   GamePadPropertyIsKnown
		       | GamePadPropertyHasLED
		       | GamePadPropertyHasRumble
		       | GamePadPropertyHasGyroscope;
	}

	boolean SetLEDMode (TGamePadLEDMode Mode);
	boolean SetRumbleMode (TGamePadRumbleMode Mode);

private:
	void DecodeReport (const u8 *pReportBuffer);

	boolean PS3Enable (void);

private:
	boolean m_bInterfaceOK;

	DMA_BUFFER (u8, m_CommandBuffer, USB_GAMEPAD_PS3_COMMAND_LENGTH);

	static const u8 s_CommandDefault[];
};

#endif
