//
// keyboardbehaviour.h
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
#ifndef _circle_input_keyboardbehaviour_h
#define _circle_input_keyboardbehaviour_h

#include <circle/input/keymap.h>
#include <circle/timer.h>
#include <circle/types.h>

enum TModifierKey
{
	ModifierKeyLeftCtrl = 0x80,
	ModifierKeyLeftShift,
	ModifierKeyAlt,
	ModifierKeyLeftWin,
	ModifierKeyRightCtrl,
	ModifierKeyRightShift,
	ModifierKeyAltGr,
	ModifierKeyRightWin,
	ModifierKeyUnknown
};

#define KEY_LCTRL_MASK		(1 << 0)
#define KEY_LSHIFT_MASK		(1 << 1)
#define KEY_ALT_MASK		(1 << 2)
#define KEY_LWIN_MASK		(1 << 3)
#define KEY_RCTRL_MASK		(1 << 4)
#define KEY_RSHIFT_MASK		(1 << 5)
#define KEY_ALTGR_MASK		(1 << 6)
#define KEY_RWIN_MASK		(1 << 7)

#define KEYB_LED_NUM_LOCK	(1 << 0)
#define KEYB_LED_CAPS_LOCK	(1 << 1)
#define KEYB_LED_SCROLL_LOCK	(1 << 2)

typedef void TKeyPressedHandler (const char *pString);
typedef void TSelectConsoleHandler (unsigned nConsole);
typedef void TShutdownHandler (void);

class CKeyboardBehaviour
{
public:
	CKeyboardBehaviour (void);
	~CKeyboardBehaviour (void);

	void RegisterKeyPressedHandler (TKeyPressedHandler *pKeyPressedHandler);
	void RegisterSelectConsoleHandler (TSelectConsoleHandler *pSelectConsoleHandler);
	void RegisterShutdownHandler (TShutdownHandler *pShutdownHandler);

	void KeyPressed (u8 ucKeyCode);
	void KeyReleased (u8 ucKeyCode);

	u8 GetLEDStatus (void) const;

private:
	void GenerateKeyEvent (u8 ucKeyCode);

	void TimerHandler (TKernelTimerHandle hTimer);
	static void TimerStub (TKernelTimerHandle hTimer, void *pParam, void *pContext);

private:
	TKeyPressedHandler	*m_pKeyPressedHandler;
	TSelectConsoleHandler	*m_pSelectConsoleHandler;
	TShutdownHandler	*m_pShutdownHandler;

	u8 m_ucModifiers;
	u8 m_ucLastKeyCode;

	TKernelTimerHandle m_hTimer;

	CKeyMap m_KeyMap;
};

#endif
