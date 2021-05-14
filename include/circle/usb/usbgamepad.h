//
// usbgamepad.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbgamepad_h
#define _circle_usb_usbgamepad_h

#include <circle/usb/usbhiddevice.h>
#include <circle/numberpool.h>
#include <circle/macros.h>
#include <circle/types.h>

enum TGamePadProperty
{
	GamePadPropertyIsKnown		     = BIT(0),	// has known mapping of controls
	GamePadPropertyHasLED		     = BIT(1),	// supports SetLEDMode()
	GamePadPropertyHasRGBLED	     = BIT(2),	// if set, GamePadPropertyHasLED is set too
	GamePadPropertyHasRumble	     = BIT(3),	// supports SetRumbleMode()
	GamePadPropertyHasGyroscope	     = BIT(4),	// provides sensor info in TGamePadState
	GamePadPropertyHasTouchpad	     = BIT(5),	// has touchpad with button
	GamePadPropertyHasAlternativeMapping = BIT(6)	// additional +/- buttons, no START button
};

// The following enums are valid for known gamepads only!

enum TGamePadButton		// Digital button (bit masks)
{
	GamePadButtonGuide	= BIT(0),
#define GamePadButtonXbox	GamePadButtonGuide
#define GamePadButtonPS		GamePadButtonGuide
#define GamePadButtonHome	GamePadButtonGuide
	GamePadButtonLT		= BIT(3),
#define GamePadButtonL2		GamePadButtonLT
#define GamePadButtonLZ		GamePadButtonLT
	GamePadButtonRT		= BIT(4),
#define GamePadButtonR2		GamePadButtonRT
#define GamePadButtonRZ		GamePadButtonRT
	GamePadButtonLB		= BIT(5),
#define GamePadButtonL1		GamePadButtonLB
#define GamePadButtonL		GamePadButtonLB
	GamePadButtonRB		= BIT(6),
#define GamePadButtonR1		GamePadButtonRB
#define GamePadButtonR		GamePadButtonRB
	GamePadButtonY		= BIT(7),
#define GamePadButtonTriangle	GamePadButtonY
	GamePadButtonB		= BIT(8),
#define GamePadButtonCircle	GamePadButtonB
	GamePadButtonA		= BIT(9),
#define GamePadButtonCross	GamePadButtonA
	GamePadButtonX		= BIT(10),
#define GamePadButtonSquare	GamePadButtonX
	GamePadButtonSelect	= BIT(11),
#define GamePadButtonBack	GamePadButtonSelect
#define GamePadButtonShare	GamePadButtonSelect
#define GamePadButtonCapture	GamePadButtonSelect
	GamePadButtonL3		= BIT(12),		// Left axis button
#define GamePadButtonSL		GamePadButtonL3
	GamePadButtonR3		= BIT(13),		// Right axis button
#define GamePadButtonSR		GamePadButtonR3
	GamePadButtonStart	= BIT(14),		// optional
#define GamePadButtonOptions	GamePadButtonStart
	GamePadButtonUp		= BIT(15),
	GamePadButtonRight	= BIT(16),
	GamePadButtonDown	= BIT(17),
	GamePadButtonLeft	= BIT(18),
	GamePadButtonPlus	= BIT(19),		// optional
	GamePadButtonMinus	= BIT(20),		// optional
	GamePadButtonTouchpad	= BIT(21)		// optional
};

// Number of digital buttons for known gamepads:
#define GAMEPAD_BUTTONS_STANDARD	19
#define GAMEPAD_BUTTONS_ALTERNATIVE	21
#define GAMEPAD_BUTTONS_WITH_TOUCHPAD	22

enum TGamePadAxis		// Axis or analog button
{
	GamePadAxisLeftX,
	GamePadAxisLeftY,
	GamePadAxisRightX,
	GamePadAxisRightY,
	GamePadAxisButtonLT,
#define GamePadAxisButtonL2	GamePadAxisButtonLT
	GamePadAxisButtonRT,
#define GamePadAxisButtonR2	GamePadAxisButtonRT
	GamePadAxisButtonUp,
	GamePadAxisButtonRight,
	GamePadAxisButtonDown,
	GamePadAxisButtonLeft,
	GamePadAxisButtonL1,
	GamePadAxisButtonR1,
	GamePadAxisButtonTriangle,
	GamePadAxisButtonCircle,
	GamePadAxisButtonCross,
	GamePadAxisButtonSquare,
	GamePadAxisUnknown
};

enum TGamePadLEDMode
{
	GamePadLEDModeOff,
	GamePadLEDModeOn1,
	GamePadLEDModeOn2,
	GamePadLEDModeOn3,
	GamePadLEDModeOn4,
	GamePadLEDModeOn5,
	GamePadLEDModeOn6,
	GamePadLEDModeOn7,
	GamePadLEDModeOn8,
	GamePadLEDModeOn9,
	GamePadLEDModeOn10,
	GamePadLEDModeUnknown
};

enum TGamePadRumbleMode
{
	GamePadRumbleModeOff,
	GamePadRumbleModeLow,
	GamePadRumbleModeHigh,
	GamePadRumbleModeUnknown
};

#define MAX_AXIS    16
#define MAX_HATS    6

struct TGamePadState		/// Current state of a gamepad
{
	int naxes;		// Number of available axes and analog buttons
	struct
	{
		int value;	// Current position value
		int minimum;	// Minimum position value (normally 0)
		int maximum;	// Maximum position value (normally 255)
	}
	axes[MAX_AXIS];		// Array of axes and analog buttons

	int nhats;		// Number of available hat controls
	int hats[MAX_HATS];	// Current position value of hat controls

	int nbuttons;		// Number of available digital buttons
	unsigned buttons;	// Current status of digital buttons (bit mask)

	int acceleration[3];	// Current state of acceleration sensor (x, y, z)
	int gyroscope[3];	// Current state of gyroscope sensor (x, y, z)
};

#define GAMEPAD_AXIS_DEFAULT_MINIMUM	0
#define GAMEPAD_AXIS_DEFAULT_MAXIMUM	255

typedef void TGamePadStatusHandler (unsigned nDeviceIndex, const TGamePadState *pGamePadState);

class CUSBGamePadDevice : public CUSBHIDDevice		/// Base class for USB gamepad drivers
{
public:
	CUSBGamePadDevice (CUSBFunction *pFunction);
	virtual ~CUSBGamePadDevice (void);

	boolean Configure (void);

	/// \return Properties of the gamepad (bit mask of TGamePadProperty constants)
	virtual unsigned GetProperties (void) { return 0; }

	/// \return Pointer to gamepad state (with the control numbers set)
	/// \note The controls state fields may have some default value.
	virtual const TGamePadState *GetInitialState (void);
	/// \brief Same as GetInitialState() for compatibility (deprecated)
	const TGamePadState *GetReport (void) { return GetInitialState (); }

	/// \param pStatusHandler Pointer to the function to be called on status changes
	virtual void RegisterStatusHandler (TGamePadStatusHandler *pStatusHandler);

public:
	/// \brief Set LED(s) on gamepads with multiple uni-color LEDs
	/// \param Mode LED mode to be set
	/// \return LED mode supported and successful set?
	/// \note A gamepad may support only a subset of the defined TGamePadLEDMode modes.
	virtual boolean SetLEDMode (TGamePadLEDMode Mode) { return FALSE; }

	/// \brief Set LED(s) on gamepads with a single flash-able RGB-color LED
	/// \param nRGB       Color value set be set (0x00rrggbb)
	/// \param uchTimeOn  Duration while the LED is on (1/100 seconds)
	/// \param uchTimeOff Duration while the LED is off (1/100 seconds)
	/// \return Operation successful?
	virtual boolean SetLEDMode (u32 nRGB, u8 uchTimeOn, u8 uchTimeOff) { return FALSE; }

	/// \param Mode Rumble mode to be set
	/// \return Operation successful?
	virtual boolean SetRumbleMode (TGamePadRumbleMode Mode) { return FALSE; }

private:
	/// \param pReport Pointer to report packet received via the USB status reporting endpoint
	/// \param nReportSize Size of the received report in number of bytes
	/// \note Overwrite this if you have to do additional checks on received reports!
	virtual void ReportHandler (const u8 *pReport, unsigned nReportSize);

	/// \param pReportBuffer Pointer to report packet received via the USB status reporting endpoint
	/// \note Updates the m_State member variable
	/// \note m_usReportSize member has to be set here or in Configure() of the subclass.
	virtual void DecodeReport (const u8 *pReportBuffer) = 0;

protected:
	TGamePadState m_State;
	TGamePadStatusHandler *m_pStatusHandler;

	u16 m_usReportSize;

	unsigned m_nDeviceNumber;
	static CNumberPool s_DeviceNumberPool;
};

#endif
