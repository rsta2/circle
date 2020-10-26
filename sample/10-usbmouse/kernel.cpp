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
#include <assert.h>

#if DEPTH == 8

#define NUM_COLORS	3

TScreenColor CKernel::s_Colors[NUM_COLORS] =
{
	HIGH_COLOR,
	HALF_COLOR,
	NORMAL_COLOR
};

#elif DEPTH == 16

#define NUM_COLORS	7

TScreenColor CKernel::s_Colors[NUM_COLORS] =
{
	COLOR16 (31, 0, 0),
	COLOR16 (0, 31, 0),
	COLOR16 (0, 0, 31),
	COLOR16 (31, 31, 0),
	COLOR16 (0, 31, 31),
	COLOR16 (31, 0, 31),
	COLOR16 (31, 31, 31)
};

#elif DEPTH == 32

#define NUM_COLORS	7

TScreenColor CKernel::s_Colors[NUM_COLORS] =
{
	COLOR32 (255U, 0, 0, 255U),
	COLOR32 (0, 255U, 0, 255U),
	COLOR32 (0, 0, 255U, 255U),
	COLOR32 (255U, 255U, 0, 255U),
	COLOR32 (0, 255U, 255U, 255U),
	COLOR32 (255U, 0, 255U, 255U),
	COLOR32 (255U, 255U, 255U, 255U)
};

#else
	#error DEPTH must be 8, 16 or 32
#endif

static const char ClearScreen[] = "\x1b[H\x1b[J\x1b[?25l";

static const char FromKernel[] = "kernel";

CKernel *CKernel::s_pThis = 0;

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer, TRUE),		// TRUE: enable plug-and-play
	m_pMouse (0),
	m_nPosX (0),
	m_nPosY (0),
	m_nColorIndex (0),
	m_Color (s_Colors[m_nColorIndex]),
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

	m_Logger.Write (FromKernel, LogNotice, "Please attach an USB mouse, if not already done!");

	for (unsigned nCount = 0; m_ShutdownMode == ShutdownNone; nCount++)
	{
		// This must be called from TASK_LEVEL to update the tree of connected USB devices.
		boolean bUpdated = m_USBHCI.UpdatePlugAndPlay ();

		if (   bUpdated
		    && m_pMouse == 0)
		{
			m_pMouse = (CMouseDevice *) m_DeviceNameService.GetDevice ("mouse1", FALSE);
			if (m_pMouse != 0)
			{
				m_pMouse->RegisterRemovedHandler (MouseRemovedHandler);

				m_Logger.Write (FromKernel, LogNotice, "USB mouse has %d buttons",
						m_pMouse->GetButtonCount());
				m_Logger.Write (FromKernel, LogNotice, "USB mouse has %s wheel",
						m_pMouse->HasWheel() ? "a" : "no");

				m_Screen.Write (ClearScreen, sizeof ClearScreen-1);

				if (!m_pMouse->Setup (m_Screen.GetWidth (), m_Screen.GetHeight ()))
				{
					m_Logger.Write (FromKernel, LogPanic, "Cannot setup mouse");
				}

				m_nPosX = m_Screen.GetWidth () / 2;
				m_nPosY = m_Screen.GetHeight () / 2;

				m_pMouse->SetCursor (m_nPosX, m_nPosY);
				m_pMouse->ShowCursor (TRUE);

				m_pMouse->RegisterEventHandler (MouseEventStub);
			}
		}

		if (m_pMouse != 0)
		{
			m_pMouse->UpdateCursor ();
		}

		m_Screen.Rotor (0, nCount);
	}

	return m_ShutdownMode;
}

void CKernel::MouseEventHandler (TMouseEvent Event, unsigned nButtons, unsigned nPosX, unsigned nPosY, int nWheelMove)
{
	switch (Event)
	{
	case MouseEventMouseMove:
		if (nButtons & (MOUSE_BUTTON_LEFT | MOUSE_BUTTON_RIGHT))
		{
			DrawLine (m_nPosX, m_nPosY, nPosX, nPosY,
				  nButtons & MOUSE_BUTTON_LEFT ? NORMAL_COLOR : m_Color);
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

	case MouseEventMouseWheel:
		m_nColorIndex += nWheelMove;
		if (m_nColorIndex < 0)
		{
			m_nColorIndex = 0;
		}
		else if (m_nColorIndex >= NUM_COLORS)
		{
			m_nColorIndex = NUM_COLORS-1;
		}

		m_Color = s_Colors[m_nColorIndex];
		break;

	default:
		break;
	}
}

void CKernel::MouseEventStub (TMouseEvent Event, unsigned nButtons, unsigned nPosX, unsigned nPosY, int nWheelMove)
{
	assert (s_pThis != 0);
	s_pThis->MouseEventHandler (Event, nButtons, nPosX, nPosY, nWheelMove);
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

void CKernel::MouseRemovedHandler (CDevice *pDevice, void *pContext)
{
	assert (s_pThis != 0);

	static const char ClearScreen[] = "\x1b[H\x1b[J\x1b[?25h";
	s_pThis->m_Screen.Write (ClearScreen, sizeof ClearScreen-1);

	CLogger::Get ()->Write (FromKernel, LogDebug, "Mouse removed");

	s_pThis->m_pMouse = 0;

	CLogger::Get ()->Write (FromKernel, LogNotice, "Please attach an USB mouse!");
}
