//
// usbgamepadps3.h
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
#ifndef _circle_usb_usbgamepadps3_h
#define _circle_usb_usbgamepadps3_h

#include <circle/usb/usbgamepadstandard.h>
#include <circle/types.h>

enum TPS3GamePadButton
{
	PS3GamePadButtonPS,
	PS3GamePadButtonReserved1,
	PS3GamePadButtonReserved2,
	PS3GamePadButtonL2,
	PS3GamePadButtonR2,
	PS3GamePadButtonL1,
	PS3GamePadButtonR1,
	PS3GamePadButtonTriangle,
	PS3GamePadButtonCircle,
	PS3GamePadButtonCross,
	PS3GamePadButtonSquare,
	PS3GamePadButtonSelect,
	PS3GamePadButtonL3,
	PS3GamePadButtonR3,
	PS3GamePadButtonStart,
	PS3GamePadButtonUp,
	PS3GamePadButtonRight,
	PS3GamePadButtonDown,
	PS3GamePadButtonLeft,
	PS3GamePadButtonUnknown
};

enum TPS3GamePadAxes
{
	PS3GamePadAxesLeftX,
	PS3GamePadAxesLeftY,
	PS3GamePadAxesRightX,
	PS3GamePadAxesRightY,
	PS3GamePadAxesButtonUp,
	PS3GamePadAxesButtonRight,
	PS3GamePadAxesButtonDown,
	PS3GamePadAxesButtonLeft,
	PS3GamePadAxesButtonL2,
	PS3GamePadAxesButtonR2,
	PS3GamePadAxesButtonL1,
	PS3GamePadAxesButtonR1,
	PS3GamePadAxesButtonTriangle,
	PS3GamePadAxesButtonCircle,
	PS3GamePadAxesButtonCross,
	PS3GamePadAxesButtonSquare,
	PS3GamePadAxesButtonUnknown
};

class CUSBGamePadPS3Device : public CUSBGamePadStandardDevice
{
public:
	CUSBGamePadPS3Device (CUSBFunction *pFunction);
	~CUSBGamePadPS3Device (void);

	boolean Configure (void);

private:
	void DecodeReport (const u8 *pReportBuffer);

	boolean PS3Enable (void);

private:
	boolean m_bInterfaceOK;
};

#endif
