//
// keyboardbuffer.cpp
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
#include <circle/input/keyboardbuffer.h>
#include <assert.h>

CKeyboardBuffer *CKeyboardBuffer::s_pThis = 0;

CKeyboardBuffer::CKeyboardBuffer (CUSBKeyboardDevice *pKeyboard)
:	m_pKeyboard (pKeyboard),
	m_nInPtr (0),
	m_nOutPtr (0)
{
	assert (s_pThis == 0);
	s_pThis = this;

	assert (m_pKeyboard != 0);
	m_pKeyboard->RegisterKeyPressedHandler (KeyPressedStub);
}

CKeyboardBuffer::~CKeyboardBuffer (void)
{
	m_pKeyboard = 0;

	s_pThis = 0;
}

int CKeyboardBuffer::Read (void *pBuffer, size_t nCount)
{
	assert (pBuffer != 0);
	char *p = (char *) pBuffer;

	int nResult = 0;

	m_SpinLock.Acquire ();

	while (   nCount > 0
	       && BufStat ())
	{
		*p++ = OutBuf ();

		nCount--;
		nResult++;
	}

	m_SpinLock.Release ();

	assert (m_pKeyboard != 0);
	m_pKeyboard->UpdateLEDs ();

	return nResult;
}

void CKeyboardBuffer::InBuf (char chChar)
{
	if (((m_nInPtr+1) & KEYB_BUF_MASK) != m_nOutPtr)
	{
		m_Buffer[m_nInPtr] = chChar;

		m_nInPtr = (m_nInPtr+1) & KEYB_BUF_MASK;
	}
}

boolean CKeyboardBuffer::BufStat (void) const
{
	return m_nInPtr != m_nOutPtr ? TRUE : FALSE;
}

char CKeyboardBuffer::OutBuf (void)
{
	if (m_nInPtr == m_nOutPtr)
	{
		return '\0';
	}

	char chChar = m_Buffer[m_nOutPtr];

	m_nOutPtr = (m_nOutPtr+1) & KEYB_BUF_MASK;

	return chChar;
}

void CKeyboardBuffer::KeyPressedHandler (const char *pString)
{
	m_SpinLock.Acquire ();

	while (*pString)
	{
		InBuf (*pString++);
	}

	m_SpinLock.Release ();
}

void CKeyboardBuffer::KeyPressedStub (const char *pString)
{
	assert (s_pThis != 0);
	s_pThis->KeyPressedHandler (pString);
}
