//
// usbgamepadxbox360pcwireless.h
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
#ifndef _circle_usb_usbgamepadxbox360pcwireless_h
#define _circle_usb_usbgamepadxbox360pcwireless_h

#include <circle/usb/usbgamepad.h>
#include <circle/types.h>

class CUSBGamePadXbox360PCWirelessDevice : public CUSBGamePadDevice
{
public:
	CUSBGamePadXbox360PCWirelessDevice (CUSBFunction *pFunction);
	~CUSBGamePadXbox360PCWirelessDevice (void);

	boolean Configure (void);

	unsigned GetProperties (void)
	{
		return   GamePadPropertyIsKnown
		       | GamePadPropertyHasLED
		       | GamePadPropertyHasRumble;
	}

	boolean SetLEDMode (TGamePadLEDMode Mode);
	boolean SetRumbleMode (TGamePadRumbleMode Mode);

private:
	void ReportHandler (const u8 *pReport, unsigned nReportSize);
	void DecodeReport (const u8 *pReport);
	boolean SendPresenceInquiry (void);

private:
	boolean m_bInterfaceOK;
	boolean m_bControllerPresent;
};

#endif
