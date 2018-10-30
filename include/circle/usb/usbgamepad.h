//
// usbgamepad.h
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
#ifndef _circle_usb_usbgamepad_h
#define _circle_usb_usbgamepad_h

#include <circle/usb/usbhiddevice.h>
#include <circle/types.h>

#define MAX_AXIS    6
#define MAX_HATS    6

struct TGamePadState
{
	int naxes;
	struct
	{
		int value;
		int minimum;
		int maximum;
	}
	axes[MAX_AXIS];

	int nhats;
	int hats[MAX_HATS];

	int nbuttons;
	unsigned buttons;
};

typedef void TGamePadStatusHandler (unsigned nDeviceIndex, const TGamePadState *pGamePadState);

class CUSBGamePadDevice : public CUSBHIDDevice		/// Base class for USB gamepad drivers
{
public:
	CUSBGamePadDevice (CUSBFunction *pFunction);
	virtual ~CUSBGamePadDevice (void);

	boolean Configure (void);

	/// \return Pointer to gamepad state (or 0 on failure)
	/// \note May return the gamepad geometry information only (not the values).
	virtual const TGamePadState *GetReport (void) = 0;

	/// \param pStatusHandler Pointer to the function to be called on status changes
	virtual void RegisterStatusHandler (TGamePadStatusHandler *pStatusHandler);

private:
	/// \param pReport Pointer to report packet received via the USB status reporting endpoint
	/// \note Overwrite this if you have to do additional checks on received reports!
	virtual void ReportHandler (const u8 *pReport);

	/// \param pReport Pointer to report packet received via the USB status reporting endpoint
	/// \note Updates the m_State member variable
	/// \note m_usReportSize member has to be set here or in Configure() of the subclass.
	virtual void DecodeReport (const u8 *pReportBuffer) = 0;

protected:
	TGamePadState m_State;
	TGamePadStatusHandler *m_pStatusHandler;

	u16 m_usReportSize;

	unsigned m_nDeviceNumber;
	static unsigned s_nDeviceNumber;
};

#endif
