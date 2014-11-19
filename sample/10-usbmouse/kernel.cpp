//
// kernel.cpp
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
#include "kernel.h"
#include <circle/usb/usbmouse.h>
#include <assert.h>

static const char ClearScreen[] = "\x1b[H\x1b[J\x1b[?25l";

static const char FromKernel[] = "kernel";

CKernel *CKernel::s_pThis = 0;

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_DWHCI (&m_Interrupt, &m_Timer),
	m_nPosX (0),
	m_nPosY (0),
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
		bOK = m_DWHCI.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	CUSBMouseDevice *pMouse = (CUSBMouseDevice *) m_DeviceNameService.GetDevice ("umouse1", FALSE);
	if (pMouse == 0)
	{
		m_Logger.Write (FromKernel, LogPanic, "Mouse not found");
	}

	m_Screen.Write (ClearScreen, sizeof ClearScreen-1);

	m_nPosX = m_Screen.GetWidth () / 2;
	m_nPosY = m_Screen.GetHeight () / 2;

	pMouse->RegisterStatusHandler (MouseStatusStub);

	// just wait and turn the rotor
	for (unsigned nCount = 0; m_ShutdownMode == ShutdownNone; nCount++)
	{
		m_Screen.Rotor (0, nCount);
		m_Timer.MsDelay (100);
	}

	return m_ShutdownMode;
}

void CKernel::MouseStatusHandler (unsigned nButtons, int nDisplacementX, int nDisplacementY)
{
	if (nButtons & MOUSE_BUTTON_MIDDLE)
	{
		m_Screen.Write (ClearScreen, sizeof ClearScreen-1);
	}
	else
	{
		m_nPosX += nDisplacementX;
		if ((unsigned) m_nPosX >= m_Screen.GetWidth ())
		{
			m_nPosX -= nDisplacementX;
		}

		m_nPosY += nDisplacementY;
		if ((unsigned) m_nPosY >= m_Screen.GetHeight ())
		{
			m_nPosY -= nDisplacementY;
		}

		if (nButtons & (MOUSE_BUTTON_LEFT | MOUSE_BUTTON_RIGHT))
		{
			m_Screen.SetPixel ((unsigned) m_nPosX, (unsigned) m_nPosY,
					   nButtons & MOUSE_BUTTON_LEFT ? NORMAL_COLOR : HIGH_COLOR);
		}
	}
}

void CKernel::MouseStatusStub (unsigned nButtons, int nDisplacementX, int nDisplacementY)
{
	assert (s_pThis != 0);
	s_pThis->MouseStatusHandler (nButtons, nDisplacementX, nDisplacementY);
}
