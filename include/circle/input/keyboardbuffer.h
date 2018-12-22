//
// keyboardbuffer.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2018  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_input_keyboardbuffer_h
#define _circle_input_keyboardbuffer_h

#include <circle/device.h>
#include <circle/usb/usbkeyboard.h>
#include <circle/spinlock.h>
#include <circle/types.h>

#define KEYB_BUF_SIZE		64			// must be a power of 2
#define KEYB_BUF_MASK		(KEYB_BUF_SIZE-1)

class CKeyboardBuffer : public CDevice
{
public:
	CKeyboardBuffer (CUSBKeyboardDevice *pKeyboard);
	~CKeyboardBuffer (void);

	int Read (void *pBuffer, size_t nCount);

private:
	void InBuf (char chChar);
	boolean BufStat (void) const;
	char OutBuf (void);				// returns '\0' if no key is waiting

	void KeyPressedHandler (const char *pString);
	static void KeyPressedStub (const char *pString);

private:
	CUSBKeyboardDevice *m_pKeyboard;

	char 	 m_Buffer[KEYB_BUF_SIZE];
	unsigned m_nInPtr;
	unsigned m_nOutPtr;

	CSpinLock m_SpinLock;

	static CKeyboardBuffer *s_pThis;
};

#endif
