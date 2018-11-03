//
// usbgamepadxbox360.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbgamepadxbox360_h
#define _circle_usb_usbgamepadxbox360_h

#include <circle/usb/usbgamepad.h>
#include <circle/types.h>

enum TXbox360GamePadButton
{
	Xbox360GamePadButtonUp,
	Xbox360GamePadButtonDown,
	Xbox360GamePadButtonLeft,
	Xbox360GamePadButtonRight,
	Xbox360GamePadButtonStart,
	Xbox360GamePadButtonBack,
	Xbox360GamePadButtonL3,
	Xbox360GamePadButtonR3,
	Xbox360GamePadButtonLB,
	Xbox360GamePadButtonRB,
	Xbox360GamePadButtonXbox,
	Xbox360GamePadButtonReserved,
	Xbox360GamePadButtonA,
	Xbox360GamePadButtonB,
	Xbox360GamePadButtonX,
	Xbox360GamePadButtonY,
	Xbox360GamePadButtonLT,
	Xbox360GamePadButtonRT,
	Xbox360GamePadButtonUnknown
};

enum TXbox360GamePadAxes
{
	Xbox360GamePadAxesLeftX,
	Xbox360GamePadAxesLeftY,
	Xbox360GamePadAxesRightX,
	Xbox360GamePadAxesRightY,
	Xbox360GamePadAxesButtonLT,
	Xbox360GamePadAxesButtonRT,
	Xbox360GamePadAxesButtonUnknown
};

class CUSBGamePadXbox360Device : public CUSBGamePadDevice
{
public:
	CUSBGamePadXbox360Device (CUSBFunction *pFunction);
	~CUSBGamePadXbox360Device (void);

	boolean Configure (void);

	const TGamePadState *GetReport (void);

private:
	void ReportHandler (const u8 *pReport);

	void DecodeReport (const u8 *pReport);
};

#endif
