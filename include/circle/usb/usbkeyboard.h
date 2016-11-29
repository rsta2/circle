//
// usbkeyboard.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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
#ifndef _usbkeyboard_h
#define _usbkeyboard_h

#include <circle/usb/usbhiddevice.h>
#include <circle/input/keyboardbehaviour.h>
#include <circle/usb/usbhid.h>
#include <circle/types.h>

#define USBKEYB_REPORT_SIZE	8

// The raw handler is called when the keyboard sends a status report (on status change and/or continously).
typedef void TKeyStatusHandlerRaw (unsigned char	ucModifiers,	// see usbhid.h
				   const unsigned char	RawKeys[6]);	// key code or 0 in each byte

class CUSBKeyboardDevice : public CUSBHIDDevice
{
public:
	CUSBKeyboardDevice (CUSBFunction *pFunction);
	~CUSBKeyboardDevice (void);

	boolean Configure (void);

	// cooked mode
	void RegisterKeyPressedHandler (TKeyPressedHandler *pKeyPressedHandler);
	void RegisterSelectConsoleHandler (TSelectConsoleHandler *pSelectConsoleHandler);
	void RegisterShutdownHandler (TShutdownHandler *pShutdownHandler);

	u8 GetLEDStatus (void) const;	// returns USB LED status to be handed-over to SetLEDs()

	// raw mode (if this handler is registered the others are ignored)
	void RegisterKeyStatusHandlerRaw (TKeyStatusHandlerRaw *pKeyStatusHandlerRaw);

	// works in cooked and raw mode
	boolean SetLEDs (u8 ucStatus);		// must not be called in interrupt context

private:
	void ReportHandler (const u8 *pReport);

	static boolean FindByte (const u8 *pBuffer, u8 ucByte, unsigned nLength);

private:
	CKeyboardBehaviour m_Behaviour;

	TKeyStatusHandlerRaw *m_pKeyStatusHandlerRaw;

	u8 m_LastReport[USBKEYB_REPORT_SIZE];

	static unsigned s_nDeviceNumber;
};

#endif
