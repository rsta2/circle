//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#include <circle/string.h>
#include <circle/util.h>
#include <assert.h>

static const char FromKernel[] = "kernel";

CKernel *CKernel::s_pThis = 0;

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer, TRUE),		// TRUE: enable plug-and-play
	m_pKeyboard (0),
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
		bOK = m_Serial.Initialize (115200);
	}

	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Screen;
		}

		bOK = m_Logger.Initialize (pTarget);
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
		bOK = m_USBHCI.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	m_Logger.Write (FromKernel, LogNotice, "Please attach an USB keyboard, if not already done!");

	for (unsigned nCount = 0; m_ShutdownMode == ShutdownNone; nCount++)
	{
		// This must be called from TASK_LEVEL to update the tree of connected USB devices.
		boolean bUpdated = m_USBHCI.UpdatePlugAndPlay ();

		if (   bUpdated
		    && m_pKeyboard == 0)
		{
			m_pKeyboard = (CUSBKeyboardDevice *) m_DeviceNameService.GetDevice ("ukbd1", FALSE);
			if (m_pKeyboard != 0)
			{
				m_pKeyboard->RegisterRemovedHandler (KeyboardRemovedHandler);

#if 1	// set to 0 to test raw mode
				m_pKeyboard->RegisterShutdownHandler (ShutdownHandler);
				m_pKeyboard->RegisterKeyPressedHandler (KeyPressedHandler);
#else
				m_pKeyboard->RegisterKeyStatusHandlerRaw (KeyStatusHandlerRaw);
#endif

				m_Logger.Write (FromKernel, LogNotice, "Just type something!");
			}
		}

		if (m_pKeyboard != 0)
		{
			// CUSBKeyboardDevice::UpdateLEDs() must not be called in interrupt context,
			// that's why this must be done here. This does nothing in raw mode.
			m_pKeyboard->UpdateLEDs ();
		}

		m_Screen.Rotor (0, nCount);
		m_Timer.MsDelay (100);
	}

	return m_ShutdownMode;
}

void CKernel::KeyPressedHandler (const char *pString)
{
	assert (s_pThis != 0);
	s_pThis->m_Screen.Write (pString, strlen (pString));
}

void CKernel::ShutdownHandler (void)
{
	assert (s_pThis != 0);
	s_pThis->m_ShutdownMode = ShutdownReboot;
}

void CKernel::KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6], void* arg)
{
	assert (s_pThis != 0);

	CString Message;
	Message.Format ("Key status (modifiers %02X)", (unsigned) ucModifiers);

	for (unsigned i = 0; i < 6; i++)
	{
		if (RawKeys[i] != 0)
		{
			CString KeyCode;
			KeyCode.Format (" %02X", (unsigned) RawKeys[i]);

			Message.Append (KeyCode);
		}
	}

	s_pThis->m_Logger.Write (FromKernel, LogNotice, Message);
}

void CKernel::KeyboardRemovedHandler (CDevice *pDevice, void *pContext)
{
	assert (s_pThis != 0);

	CLogger::Get ()->Write (FromKernel, LogDebug, "Keyboard removed");

	s_pThis->m_pKeyboard = 0;
}
