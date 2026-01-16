//
// keyboardbehaviour.cpp
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
#include <circle/input/keyboardbehaviour.h>
#include <assert.h>

//#define REPEAT_ENABLE

#define REPEAT_DELAY		MSEC2HZ (400)
#define REPEAT_RATE		MSEC2HZ (80)

CKeyboardBehaviour::CKeyboardBehaviour (void)
:	m_pKeyPressedHandler (0),
	m_pSelectConsoleHandler (0),
	m_pShutdownHandler (0),
	m_ucModifiers (0),
	m_ucLastKeyCode (0),
	m_hTimer (0)
{
}

CKeyboardBehaviour::~CKeyboardBehaviour (void)
{
	m_pKeyPressedHandler = 0;
	m_pSelectConsoleHandler = 0;
	m_pShutdownHandler = 0;
}

void CKeyboardBehaviour::RegisterKeyPressedHandler (TKeyPressedHandler *pKeyPressedHandler)
{
	assert (pKeyPressedHandler != 0);
	m_pKeyPressedHandler = pKeyPressedHandler;
}

void CKeyboardBehaviour::RegisterSelectConsoleHandler (TSelectConsoleHandler *pSelectConsoleHandler)
{
	assert (pSelectConsoleHandler != 0);
	m_pSelectConsoleHandler = pSelectConsoleHandler;
}

void CKeyboardBehaviour::RegisterShutdownHandler (TShutdownHandler *pShutdownHandler)
{
	assert (pShutdownHandler != 0);
	m_pShutdownHandler = pShutdownHandler;
}

u8 CKeyboardBehaviour::GetLEDStatus (void) const
{
	return m_KeyMap.GetLEDStatus ();
}

void CKeyboardBehaviour::GenerateKeyEvent (u8 ucKeyCode)
{
	const char *pKeyString;
	char Buffer[2];

	u8 ucModifiers = m_ucModifiers;
	u16 usLogCode = m_KeyMap.Translate (ucKeyCode, ucModifiers);

	switch (usLogCode)
	{
	case ActionSwitchCapsLock:
	case ActionSwitchNumLock:
	case ActionSwitchScrollLock:
		break;

	case ActionSelectConsole1:
	case ActionSelectConsole2:
	case ActionSelectConsole3:
	case ActionSelectConsole4:
	case ActionSelectConsole5:
	case ActionSelectConsole6:
	case ActionSelectConsole7:
	case ActionSelectConsole8:
	case ActionSelectConsole9:
	case ActionSelectConsole10:
	case ActionSelectConsole11:
	case ActionSelectConsole12:
		if (m_pSelectConsoleHandler != 0)
		{
			unsigned nConsole = usLogCode - ActionSelectConsole1;
			assert (nConsole < 12);

			(*m_pSelectConsoleHandler) (nConsole);
		}
		break;

	case ActionShutdown:
		if (m_pShutdownHandler != 0)
		{
			(*m_pShutdownHandler) ();
		}
		break;

	default:
		pKeyString = m_KeyMap.GetString (usLogCode, ucModifiers, Buffer);
		if (   pKeyString != 0
		    && m_pKeyPressedHandler != 0)
		{
			(*m_pKeyPressedHandler) (pKeyString);
		}
		break;
	}
}

void CKeyboardBehaviour::KeyPressed (u8 ucKeyCode)
{
	if (0x80 <= ucKeyCode && ucKeyCode < ModifierKeyUnknown)
	{
		unsigned nMask = 1 << (ucKeyCode - 0x80);
		assert (nMask < 0x100);
		m_ucModifiers |= nMask;
	}
	else
	{
		if (ucKeyCode != m_ucLastKeyCode)
		{
			m_ucLastKeyCode = ucKeyCode;

			GenerateKeyEvent (ucKeyCode);

#ifdef REPEAT_ENABLE
			if (m_hTimer != 0)
			{
				CTimer::Get ()->CancelKernelTimer (m_hTimer);
			}

			m_hTimer = CTimer::Get ()->StartKernelTimer (REPEAT_DELAY, TimerStub, 0, this);
			assert (m_hTimer != 0);
#endif
		}
	}
}

void CKeyboardBehaviour::KeyReleased (u8 ucKeyCode)
{
	if (0x80 <= ucKeyCode && ucKeyCode < ModifierKeyUnknown)
	{
		unsigned nMask = 1 << (ucKeyCode - 0x80);
		assert (nMask < 0x100);
		m_ucModifiers &= ~nMask;
	}
	else
	{
		if (ucKeyCode == m_ucLastKeyCode)
		{
			if (m_hTimer != 0)
			{
				CTimer::Get ()->CancelKernelTimer (m_hTimer);
				m_hTimer = 0;
			}

			m_ucLastKeyCode = 0;
		}
	}
}

void CKeyboardBehaviour::TimerHandler (TKernelTimerHandle hTimer)
{
	assert (hTimer == m_hTimer);

	if (m_ucLastKeyCode != 0)
	{
		GenerateKeyEvent (m_ucLastKeyCode);

		m_hTimer = CTimer::Get ()->StartKernelTimer (REPEAT_RATE, TimerStub, 0, this);
		assert (m_hTimer != 0);
	}
}

void CKeyboardBehaviour::TimerStub (TKernelTimerHandle hTimer, void *pParam, void *pContext)
{
	CKeyboardBehaviour *pThis = (CKeyboardBehaviour *) pContext;
	assert (pThis != 0);

	pThis->TimerHandler (hTimer);
}
