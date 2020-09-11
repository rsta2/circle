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

#define DEVICE_INDEX	1		// "upad1"

CKernel *CKernel ::s_pThis = 0;

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (640, 480),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer, TRUE),		// TRUE: enable plug-and-play
	m_pGamePad (0)
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

	m_Logger.Write (FromKernel, LogNotice, "Please attach an USB gamepad, if not already done!");

	for (unsigned nCount = 0; 1; nCount++)
	{
		m_Screen.Rotor (0, nCount);
		m_Timer.MsDelay (20);

		// This must be called from TASK_LEVEL to update the tree of connected USB devices.
		boolean bUpdated = m_USBHCI.UpdatePlugAndPlay ();

		if (m_pGamePad == 0)
		{
			if (!bUpdated)
			{
				continue;
			}

			m_bGamePadKnown = FALSE;

			// get pointer to gamepad device and check if it is supported
			m_pGamePad = (CUSBGamePadDevice *)
				m_DeviceNameService.GetDevice ("upad", DEVICE_INDEX, FALSE);
			if (m_pGamePad == 0)
			{
				continue;
			}

			m_pGamePad->RegisterRemovedHandler (GamePadRemovedHandler);

			if (!(m_pGamePad->GetProperties () & GamePadPropertyIsKnown))
			{
				m_Logger.Write (FromKernel, LogWarning,
						"Gamepad mapping is not known");

				continue;
			}

			m_bGamePadKnown = TRUE;

			// get initial state from gamepad and register status handler
			const TGamePadState *pState = m_pGamePad->GetInitialState ();
			assert (pState != 0);
			GamePadStatusHandler (DEVICE_INDEX-1, pState);

			m_pGamePad->RegisterStatusHandler (GamePadStatusHandler);

			// make some rumble, if it is supported
			if (m_pGamePad->GetProperties () & GamePadPropertyHasRumble)
			{
				if (m_pGamePad->SetRumbleMode (GamePadRumbleModeLow))
				{
					m_Timer.MsDelay (250);
					m_pGamePad->SetRumbleMode (GamePadRumbleModeOff);
				}
			}

			// display stylised gamepad on screen
			static const char ClearScreen[] = "\x1b[H\x1b[J\x1b[?25l";
			m_Screen.Write (ClearScreen, sizeof ClearScreen);

			MoveTo (50, 429);
			LineTo (170, 429);
			LineTo (200, 279);
			LineTo (439, 279);
			LineTo (469, 429);
			LineTo (589, 429);
			LineTo (539, 50);
			LineTo (100, 50);
			LineTo (50, 429);
			MoveTo (92, 119);
			LineTo (547, 119);
		}

		if (!m_bGamePadKnown)
		{
			continue;
		}

		// display status of gamepad controls
		ShowAxisButton (140, 60, GamePadButtonL2, GamePadAxisButtonL2);
		ShowAxisButton (140, 110, GamePadButtonL1, GamePadAxisButtonL1);
		ShowAxisButton (477, 60, GamePadButtonR2, GamePadAxisButtonR2);
		ShowAxisButton (477, 110, GamePadButtonR1, GamePadAxisButtonR1);

		ShowAxisButton (140, 140, GamePadButtonUp, GamePadAxisButtonUp);
		ShowAxisButton (105, 160, GamePadButtonLeft, GamePadAxisButtonLeft);
		ShowAxisButton (170, 160, GamePadButtonRight, GamePadAxisButtonRight);
		ShowAxisButton (140, 180, GamePadButtonDown, GamePadAxisButtonDown);

		if (m_pGamePad->GetProperties () & GamePadPropertyHasAlternativeMapping)
		{
			ShowDigitalButton (240, 160, GamePadButtonMinus);
			ShowDigitalButton (352, 160, GamePadButtonPlus);
			ShowDigitalButton (248, 200, GamePadButtonCapture);
			ShowDigitalButton (346, 200, GamePadButtonHome);

			ShowDigitalButton (477, 140, GamePadButtonX);
			ShowDigitalButton (442, 160, GamePadButtonY);
			ShowDigitalButton (507, 160, GamePadButtonA);
			ShowDigitalButton (477, 180, GamePadButtonB);
		}
		else
		{
			ShowDigitalButton (240, 200, GamePadButtonSelect);
			ShowDigitalButton (296, 200, GamePadButtonGuide);
			ShowDigitalButton (352, 200, GamePadButtonStart);

			ShowAxisButton (477, 140, GamePadButtonTriangle, GamePadAxisButtonTriangle);
			ShowAxisButton (442, 160, GamePadButtonSquare, GamePadAxisButtonSquare);
			ShowAxisButton (507, 160, GamePadButtonCircle, GamePadAxisButtonCircle);
			ShowAxisButton (477, 180, GamePadButtonCross, GamePadAxisButtonCross);
		}

		ShowAxisControl (120, 220, GamePadAxisLeftX, GamePadAxisLeftY, GamePadButtonL3);
		ShowAxisControl (465, 220, GamePadAxisRightX, GamePadAxisRightY, GamePadButtonR3);

		if (m_pGamePad->GetProperties () & GamePadPropertyHasGyroscope)
		{
			ShowGyroscopeData (240, 323, m_GamePadState.acceleration, "ACCEL");
			ShowGyroscopeData (352, 323, m_GamePadState.gyroscope, "GYRO");
		}
	}

	return ShutdownHalt;
}

void CKernel::ShowDigitalButton (int nPosX, int nPosY, TGamePadButton Button, const char *pName)
{
	PrintAt (nPosX, nPosY, pName, m_GamePadState.buttons & Button ? TRUE : FALSE);
}

void CKernel::ShowAxisButton (int nPosX, int nPosY, TGamePadButton Button, TGamePadAxis Axis)
{
	if (Axis < m_GamePadState.naxes)
	{
		CString Value;
		Value.Format ("%03u", m_GamePadState.axes[Axis].value);

		PrintAt (nPosX, nPosY, Value, m_GamePadState.buttons & Button ? TRUE : FALSE);
	}
	else
	{
		ShowDigitalButton (nPosX-8, nPosY, Button);
	}
}

void CKernel::ShowAxisControl (int nPosX, int nPosY, TGamePadAxis AxisX,
			       TGamePadAxis AxisY, TGamePadButton Button)
{
	CString Value;
	Value.Format ("X  %03u", m_GamePadState.axes[AxisX].value);
	PrintAt (nPosX, nPosY, Value);

	ShowDigitalButton (nPosX, nPosY+20, Button);

	Value.Format ("Y  %03u", m_GamePadState.axes[AxisY].value);
	PrintAt (nPosX, nPosY+40, Value);
}

void CKernel::ShowGyroscopeData (int nPosX, int nPosY, const int Data[3], const char *pName)
{
	if (pName != 0)
	{
		PrintAt (nPosX, nPosY, pName);

		nPosY += 20;
	}

	for (unsigned i = 0; i < 3; i++, nPosY += 20)
	{
		static const char Axis[] = {'X', 'Y', 'Z'};

		CString Value;
		Value.Format ("%c %4d", Axis[i], Data[i]);

		PrintAt (nPosX, nPosY, Value);
	}
}

void CKernel::PrintAt (int nPosX, int nPosY, const char *pString, boolean bBold)
{
	if (bBold)
	{
		static const char BoldMode[] = "\x1b[1m";
		m_Screen.Write (BoldMode, sizeof BoldMode);
	}

	unsigned nCharWidth = m_Screen.GetWidth () / m_Screen.GetColumns ();
	unsigned nCharHeight = m_Screen.GetHeight () / m_Screen.GetRows ();
	nPosX /= nCharWidth;
	nPosY /= nCharHeight;

	CString Msg;
	Msg.Format ("\x1b[%d;%dH%s", nPosY+1, nPosX+1, pString);
	m_Screen.Write (Msg, Msg.GetLength ());

	if (bBold)
	{
		static const char NormalMode[] = "\x1b[0m";
		m_Screen.Write (NormalMode, sizeof NormalMode);
	}

}

void CKernel::MoveTo (int nPosX, int nPosY)
{
	m_nPosX = nPosX;
	m_nPosY = nPosY;
}

void CKernel::LineTo (int nPosX, int nPosY, TScreenColor Color)
{
	DrawLine (m_nPosX, m_nPosY, nPosX, nPosY, Color);

	MoveTo (nPosX, nPosY);
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

void CKernel::GamePadStatusHandler (unsigned nDeviceIndex, const TGamePadState *pState)
{
	if (nDeviceIndex != DEVICE_INDEX-1)
	{
		return;
	}

	assert (s_pThis != 0);
	assert (pState != 0);
	memcpy (&s_pThis->m_GamePadState, pState, sizeof *pState);
}

void CKernel::GamePadRemovedHandler (CDevice *pDevice, void *pContext)
{
	assert (s_pThis != 0);

	static const char ClearScreen[] = "\x1b[H\x1b[J\x1b[?25h";
	s_pThis->m_Screen.Write (ClearScreen, sizeof ClearScreen);

	CLogger::Get ()->Write (FromKernel, LogDebug, "Gamepad removed");

	s_pThis->m_pGamePad = 0;

	CLogger::Get ()->Write (FromKernel, LogNotice, "Please attach an USB gamepad!");
}
