//
// usbgamepad8bitdo.h
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
#ifndef _circle_usb_usbgamepad8bitdo_h
#define _circle_usb_usbgamepad8bitdo_h

#include <circle/usb/usbgamepad.h>

class CUSBGamePad8bitdoDevice : public CUSBGamePadDevice
{
public:
	CUSBGamePad8bitdoDevice (CUSBFunction *pFunction);
	~CUSBGamePad8bitdoDevice (void);

	boolean Configure (void);

	unsigned GetProperties (void)
	{
		return   GamePadPropertyIsKnown
		       | GamePadPropertyHasRumble;
	}

	boolean SetRumbleMode (TGamePadRumbleMode Mode);

protected:
	void ReportHandler (const u8 *pReport, unsigned nReportSize);

	void DecodeReport (const u8 *pReportBuffer);

private:
	boolean m_bInputSeen;
};

#endif