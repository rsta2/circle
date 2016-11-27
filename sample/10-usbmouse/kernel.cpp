//
// kernel.cpp
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
#include "kernel.h"
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

	if (!pMouse->Setup (m_Screen.GetWidth (), m_Screen.GetHeight ()))
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot setup mouse");
	}

	m_nPosX = m_Screen.GetWidth () / 2;
	m_nPosY = m_Screen.GetHeight () / 2;

	pMouse->SetCursor (m_nPosX, m_nPosY);
	pMouse->ShowCursor (TRUE);

	pMouse->RegisterEventHandler (MouseEventStub);

	// just wait and turn the rotor
	for (unsigned nCount = 0; m_ShutdownMode == ShutdownNone; nCount++)
	{
		m_Screen.Rotor (0, nCount);
		m_Timer.MsDelay (100);
	}

	return m_ShutdownMode;
}

void CKernel::MouseEventHandler (TMouseEvent Event, unsigned nButtons, unsigned nPosX, unsigned nPosY)
{
	switch (Event)
	{
	case MouseEventMouseMove:
		if (nButtons & (MOUSE_BUTTON_LEFT | MOUSE_BUTTON_RIGHT))
		{
			DrawLine (m_nPosX, m_nPosY, nPosX, nPosY,
				  nButtons & MOUSE_BUTTON_LEFT ? NORMAL_COLOR : HIGH_COLOR);
		}

		m_nPosX = nPosX;
		m_nPosY = nPosY;
		break;

	case MouseEventMouseDown:
		if (nButtons & MOUSE_BUTTON_MIDDLE)
		{
			m_Screen.Write (ClearScreen, sizeof ClearScreen-1);
		}
		break;

	default:
		break;
	}
}

void CKernel::MouseEventStub (TMouseEvent Event, unsigned nButtons, unsigned nPosX, unsigned nPosY)
{
	assert (s_pThis != 0);
	s_pThis->MouseEventHandler (Event, nButtons, nPosX, nPosY);
}

void CKernel::DrawLine (int nPosX1, int nPosY1, int nPosX2, int nPosY2, TScreenColor Color)
{
	// Bresenham algorithm

	int nDeltaX = nPosX2-nPosX1 >= 0 ? nPosX2-nPosX1 : nPosX1-nPosX2;
	int nSignX  = nPosX1 < nPosX2 ? 1 : -1;

	int nDeltaY = -(nPosY2-nPosY1 >= 0 ? nPosY2-nPosY1 : nPosY1-nPosY2);
	int nSignY  = nPosY1 < nPosY2 ? 1 : -1;

	int nError = nDeltaX + nDeltaY;

	while (1)
	{
		m_Screen.SetPixel ((unsigned) nPosX1, (unsigned) nPosY1, Color);

		if (   nPosX1 == nPosX2
		    && nPosY1 == nPosY2)
		{
			break;
		}

		int nError2 = nError + nError;

		if (nError2 > nDeltaY)
		{
			nError += nDeltaY;
			nPosX1 += nSignX;
		}

		if (nError2 < nDeltaX)
		{
			nError += nDeltaX;
			nPosY1 += nSignY;
		}
	}
}
