//
// keymap.h
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
#ifndef _circle_input_keymap_h
#define _circle_input_keymap_h

#include <circle/types.h>

#define PHY_MAX_CODE	127

#define K_NORMTAB	0
#define K_SHIFTTAB	1
#define K_ALTTAB	2
#define K_ALTSHIFTTAB	3
#define K_CTRLTAB       4

enum TSpecialKey
{
	KeyNone  = 0x00,
	KeySpace = 0x100,
	KeyEscape,
	KeyBackspace,
	KeyTabulator,
	KeyReturn,
	KeyInsert,
	KeyHome,
        KeyCtrlHome,
	KeyPageUp,
	KeyCtrlPageUp,
	KeyDelete,
	KeyEnd,
	KeyCtrlEnd,
	KeyPageDown,
	KeyCtrlPageDown,
	KeyUp,
	KeyDown,
	KeyLeft,
	KeyRight,
	KeyCtrlUp,
	KeyCtrlDown,
	KeyCtrlLeft,
	KeyCtrlRight,
	KeyF1,
	KeyF2,
	KeyF3,
	KeyF4,
	KeyF5,
	KeyF6,
	KeyF7,
	KeyF8,
	KeyF9,
	KeyF10,
	KeyF11,
	KeyF12,
	KeyApplication,
	KeyCapsLock,
	KeyPrintScreen,
	KeyScrollLock,
	KeyPause,
	KeyNumLock,
	KeyKP_Divide,
	KeyKP_Multiply,
	KeyKP_Subtract,
	KeyKP_Add,
	KeyKP_Enter,
	KeyKP_1,
	KeyKP_2,
	KeyKP_3,
	KeyKP_4,
	KeyKP_5,
	KeyKP_6,
	KeyKP_7,
	KeyKP_8,
	KeyKP_9,
	KeyKP_0,
	KeyKP_Center,
	KeyKP_Comma,
	KeyKP_Period,
	KeyMaxCode
};

enum TSpecialAction
{
	ActionSwitchCapsLock = KeyMaxCode,
	ActionSwitchNumLock,
	ActionSwitchScrollLock,
	ActionSelectConsole1,
	ActionSelectConsole2,
	ActionSelectConsole3,
	ActionSelectConsole4,
	ActionSelectConsole5,
	ActionSelectConsole6,
	ActionSelectConsole7,
	ActionSelectConsole8,
	ActionSelectConsole9,
	ActionSelectConsole10,
	ActionSelectConsole11,
	ActionSelectConsole12,
	ActionShutdown,
	ActionNone
};

class CKeyMap
{
public:
	CKeyMap (void);
	~CKeyMap (void);

	boolean ClearTable (u8 nTable);
	boolean SetEntry (u8 nTable, u8 nPhyCode, u16 nValue);

	u16 Translate (u8 nPhyCode, u8 nModifiers);
	const char *GetString (u16 nKeyCode, u8 nModifiers, char Buffer[2]) const;

	u8 GetLEDStatus (void) const;

private:
	static const void *LookupDefaultMap (const char *pLocale);

private:
	u16 m_KeyMap[PHY_MAX_CODE+1][K_CTRLTAB+1];

	boolean m_bCapsLock;
	boolean m_bNumLock;
	boolean m_bScrollLock;
	
	static const char *s_KeyStrings[KeyMaxCode-KeySpace];
	static const u16 s_DefaultMap[][PHY_MAX_CODE+1][K_CTRLTAB+1];
	static const char *s_MapDirectory[];
};

#endif
