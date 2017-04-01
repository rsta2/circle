//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2017  R. Stange <rsta2@o2online.de>
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
#include "kernel.h"
#include <circle/usb/usbkeyboard.h>
#include <circle/util.h>
#include <assert.h>

#define BAUDRATE	115200		// bits per second

#ifdef USE_RPI_STUB_AT
	#error This sample does not work with rpi_stub!
#endif

static const char FromKernel[] = "kernel";

CKernel *CKernel::s_pThis = 0;

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_Serial (&m_Interrupt),
	m_DWHCI (&m_Interrupt, &m_Timer),
	m_ShutdownMode (ShutdownNone)
{
	s_pThis = this;

	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
	s_pThis = 0;
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Logger.Initialize (&m_Screen);
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Serial.Initialize (BAUDRATE);
	}

	if (bOK)
	{
		bOK = m_DWHCI.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	CUSBKeyboardDevice *pKeyboard =
		(CUSBKeyboardDevice *) m_DeviceNameService.GetDevice ("ukbd1", FALSE);
	if (pKeyboard == 0)
	{
		m_Logger.Write (FromKernel, LogPanic, "Keyboard not found");
	}

	pKeyboard->RegisterShutdownHandler (ShutdownHandler);
	pKeyboard->RegisterKeyPressedHandler (KeyPressedHandler);

	m_Serial.SetOptions (m_Serial.GetOptions () & ~SERIAL_OPTION_ONLCR);

	m_Logger.Write (FromKernel, LogNotice, "Terminal is ready");

	for (unsigned nCount = 0; m_ShutdownMode == ShutdownNone; nCount++)
	{
		char Buffer[500];
		int nResult = m_Serial.Read (Buffer, sizeof Buffer);
		if (nResult < 0)
		{
			m_Logger.Write (FromKernel, LogWarning, "Serial read error (%d)", nResult);
		}
		else if (nResult > 0)
		{
			m_Screen.Write (Buffer, nResult);
		}

		pKeyboard->UpdateLEDs ();

		m_Screen.Rotor (0, nCount);
	}

	return m_ShutdownMode;
}

void CKernel::KeyPressedHandler (const char *pString)
{
	assert (s_pThis != 0);

	size_t nLength = strlen (pString);
	assert (nLength > 0);

	int nResult = s_pThis->m_Serial.Write (pString, nLength);
	if (nResult < (int) nLength)
	{
		s_pThis->m_Logger.Write (FromKernel, LogWarning, "Serial write error (%d)", nResult);
	}
}

void CKernel::ShutdownHandler (void)
{
	assert (s_pThis != 0);
	s_pThis->m_ShutdownMode = ShutdownReboot;
}
