//
// usbkeyboard.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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

#include <circle/usb/usbdevice.h>
#include <circle/usb/usbendpoint.h>
#include <circle/usb/usbrequest.h>
#include <circle/usb/keymap.h>
#include <circle/types.h>

#define BOOT_REPORT_SIZE	8

typedef void TKeyPressedHandler (const char *pString);
typedef void TSelectConsoleHandler (unsigned nConsole);
typedef void TShutdownHandler (void);

// The raw handler is called when the keyboard sends a status report (on status change and/or continously).
typedef void TKeyStatusHandlerRaw (unsigned char	ucModifiers,	// see usbhid.h
				   const unsigned char *pRawKeys);	// 0-terminated (max. 6 keys)

class CUSBKeyboardDevice : public CUSBDevice
{
public:
	CUSBKeyboardDevice (CUSBDevice *pDevice);
	~CUSBKeyboardDevice (void);

	boolean Configure (void);

	// cooked mode
	void RegisterKeyPressedHandler (TKeyPressedHandler *pKeyPressedHandler);
	void RegisterSelectConsoleHandler (TSelectConsoleHandler *pSelectConsoleHandler);
	void RegisterShutdownHandler (TShutdownHandler *pShutdownHandler);

	// raw mode (if this handler is registered the others are ignored)
	void RegisterKeyStatusHandlerRaw (TKeyStatusHandlerRaw *pKeyStatusHandlerRaw);

private:
	void GenerateKeyEvent (u8 ucPhyCode);
	
	boolean StartRequest (void);
	
	void CompletionRoutine (CUSBRequest *pURB);
	static void CompletionStub (CUSBRequest *pURB, void *pParam, void *pContext);

	u8 GetModifiers (void) const;
	u8 GetKeyCode (void) const;

	void TimerHandler (unsigned hTimer);
	static void TimerStub (unsigned hTimer, void *pParam, void *pContext);
	
private:
	u8 m_ucInterfaceNumber;
	u8 m_ucAlternateSetting;

	CUSBEndpoint *m_pReportEndpoint;

	TKeyPressedHandler	*m_pKeyPressedHandler;
	TSelectConsoleHandler	*m_pSelectConsoleHandler;
	TShutdownHandler	*m_pShutdownHandler;
	TKeyStatusHandlerRaw	*m_pKeyStatusHandlerRaw;

	CUSBRequest *m_pURB;
	u8 *m_pReportBuffer;

	u8 m_ucLastPhyCode;
	unsigned m_hTimer;

	CKeyMap m_KeyMap;
	
	static unsigned s_nDeviceNumber;
};

#endif
