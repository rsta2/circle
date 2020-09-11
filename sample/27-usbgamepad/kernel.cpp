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
#include <assert.h>

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer, TRUE)			// TRUE: enable plug-and-play
{
	for (unsigned i = 0; i < MAX_GAMEPADS; i++)
	{
		m_pGamePad[i] = 0;
	}

	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
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

	m_Logger.Write (FromKernel, LogNotice, "Please attach an USB gamepad, if not already done!");

	for (unsigned nCount = 0; 1; nCount++)
	{
		// This must be called from TASK_LEVEL to update the tree of connected USB devices.
		boolean bUpdated = m_USBHCI.UpdatePlugAndPlay ();

		for (unsigned nDevice = 1; bUpdated && nDevice <= MAX_GAMEPADS; nDevice++)
		{
			if (m_pGamePad[nDevice-1] != 0)
			{
				continue;
			}

			m_pGamePad[nDevice-1] = (CUSBGamePadDevice *)
				m_DeviceNameService.GetDevice ("upad", nDevice, FALSE);
			if (m_pGamePad[nDevice-1] == 0)
			{
				continue;
			}

			const TGamePadState *pState = m_pGamePad[nDevice-1]->GetInitialState ();
			assert (pState != 0);

			m_Logger.Write (FromKernel, LogNotice, "Gamepad %u: %d Button(s) %d Hat(s)",
					nDevice, pState->nbuttons, pState->nhats);

			for (int i = 0; i < pState->naxes; i++)
			{
				m_Logger.Write (FromKernel, LogNotice,
						"Gamepad %u: Axis %d: Minimum %d Maximum %d",
						nDevice, i+1, pState->axes[i].minimum,
						pState->axes[i].maximum);
			}

			m_pGamePad[nDevice-1]->RegisterRemovedHandler (GamePadRemovedHandler, this);
			m_pGamePad[nDevice-1]->RegisterStatusHandler (GamePadStatusHandler);

			m_Logger.Write (FromKernel, LogNotice, "Use your gamepad controls!");
		}

		m_Screen.Rotor (0, nCount);
		m_Timer.MsDelay (100);
	}

	return ShutdownHalt;
}

void CKernel::GamePadStatusHandler (unsigned nDeviceIndex, const TGamePadState *pState)
{
	CString Msg;
	Msg.Format ("Gamepad %u: Buttons 0x%X", nDeviceIndex+1, pState->buttons);

	CString Value;

	if (pState->naxes > 0)
	{
		Msg.Append (" Axes");

		for (int i = 0; i < pState->naxes; i++)
		{
			Value.Format (" %d", pState->axes[i].value);
			Msg.Append (Value);
		}
	}

	if (pState->nhats > 0)
	{
		Msg.Append (" Hats");

		for (int i = 0; i < pState->nhats; i++)
		{
			Value.Format (" %d", pState->hats[i]);
			Msg.Append (Value);
		}
	}

	CLogger::Get ()->Write (FromKernel, LogNotice, Msg);
}

void CKernel::GamePadRemovedHandler (CDevice *pDevice, void *pContext)
{
	CKernel *pThis = (CKernel *) pContext;
	assert (pThis != 0);

	for (unsigned i = 0; i < MAX_GAMEPADS; i++)
	{
		if (pThis->m_pGamePad[i] == (CUSBGamePadDevice *) pDevice)
		{
			CLogger::Get ()->Write (FromKernel, LogDebug, "Gamepad %u removed", i+1);

			pThis->m_pGamePad[i] = 0;

			break;
		}
	}
}
