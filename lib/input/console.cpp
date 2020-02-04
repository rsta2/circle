//
// console.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2020  R. Stange <rsta2@o2online.de>
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
#include <circle/input/console.h>
#include <circle/devicenameservice.h>
#include <circle/usb/usbkeyboard.h>
#include <circle/logger.h>
#include <assert.h>

CConsole::CConsole (CDevice *pAlternateDevice)
:	m_pAlternateDevice (pAlternateDevice),
	m_pInputDevice (0),
	m_pOutputDevice (0),
	m_bAlternateDeviceUsed (FALSE),
	m_pKeyboardBuffer (0),
	m_pLineDiscipline (0),
	m_nOptions (CONSOLE_OPTION_ICANON | CONSOLE_OPTION_ECHO)
{
}

CConsole::CConsole (CDevice *pInputDevice, CDevice *pOutputDevice)
:	m_pAlternateDevice (0),
	m_pInputDevice (pInputDevice),
	m_pOutputDevice (pOutputDevice),
	m_bAlternateDeviceUsed (FALSE),
	m_pKeyboardBuffer (0),
	m_pLineDiscipline (0),
	m_nOptions (CONSOLE_OPTION_ICANON | CONSOLE_OPTION_ECHO)
{
	assert (m_pInputDevice != 0);
	assert (m_pOutputDevice != 0);
}

CConsole::~CConsole (void)
{
	delete m_pLineDiscipline;
	m_pLineDiscipline = 0;

	delete m_pKeyboardBuffer;
	m_pKeyboardBuffer = 0;

	m_pInputDevice = 0;
	m_pOutputDevice = 0;
}

boolean CConsole::Initialize (void)
{
	if (   m_pInputDevice == 0
	    && m_pOutputDevice == 0)
	{
		CUSBKeyboardDevice *pKeyboard =
			(CUSBKeyboardDevice *) CDeviceNameService::Get ()->GetDevice ("ukbd1", FALSE);
		if (pKeyboard != 0)
		{
			assert (m_pOutputDevice == 0);
			m_pOutputDevice = CDeviceNameService::Get ()->GetDevice ("tty1", FALSE);
			if (m_pOutputDevice != 0)
			{
				assert (m_pInputDevice == 0);
				m_pInputDevice = new CKeyboardBuffer (pKeyboard);
				assert (m_pInputDevice != 0);
			}
		}

		if (   m_pInputDevice  == 0
		    || m_pOutputDevice == 0)
		{
			if (m_pAlternateDevice == 0)
			{
				CLogger::Get ()->Write ("console", LogError,
							"Keyboard or screen is not available");

				return FALSE;
			}

			m_pInputDevice = m_pAlternateDevice;
			m_pOutputDevice = m_pAlternateDevice;
			m_bAlternateDeviceUsed = TRUE;
		}
	}

	assert (m_pLineDiscipline == 0);
	m_pLineDiscipline = new CLineDiscipline (m_pInputDevice, m_pOutputDevice);
	assert (m_pLineDiscipline != 0);

	SetOptions (m_nOptions);

	CDeviceNameService::Get ()->AddDevice ("console", this, FALSE);

	return TRUE;
}

boolean CConsole::IsAlternateDeviceUsed (void) const
{
	return m_bAlternateDeviceUsed;
}

int CConsole::Read (void *pBuffer, size_t nCount)
{
	assert (m_pLineDiscipline != 0);
	return m_pLineDiscipline->Read (pBuffer, nCount);
}

int CConsole::Write (const void *pBuffer, size_t nCount)
{
	assert (m_pOutputDevice != 0);
	return m_pOutputDevice->Write (pBuffer, nCount);
}

unsigned CConsole::GetOptions (void) const
{
	return m_nOptions;
}

void CConsole::SetOptions (unsigned nOptions)
{
	m_nOptions = nOptions;

	assert (m_pLineDiscipline != 0);
	m_pLineDiscipline->SetOptionRawMode (nOptions & CONSOLE_OPTION_ICANON ? FALSE : TRUE);
	m_pLineDiscipline->SetOptionEcho (nOptions & CONSOLE_OPTION_ECHO ? TRUE : FALSE);
}
