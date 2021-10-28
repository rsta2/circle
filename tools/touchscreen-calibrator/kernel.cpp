//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/stdarg.h>
#include <assert.h>

#define POINTS		4

#define OFFSET_X	100
#define OFFSET_Y	80

static const char FromKernel[] = "kernel";

CKernel *CKernel::s_pThis = 0;

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer),
	m_pTouchScreen (0)
{
	s_pThis = this;
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
		CDevice *pScreen = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pScreen == 0)
		{
			pScreen = &m_Screen;
		}

		bOK = m_Logger.Initialize (pScreen);
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

	m_pTouchScreen = (CTouchScreenDevice *) m_DeviceNameService.GetDevice ("touch1", FALSE);
	if (m_pTouchScreen == 0)
	{
		m_Logger.Write (FromKernel, LogPanic, "Touchscreen not found");
	}

	m_pTouchScreen->RegisterEventHandler (TouchScreenEventHandler);

	struct
	{
		unsigned nScreenX;
		unsigned nScreenY;
		unsigned nTouchX;
		unsigned nTouchY;
	}
	Point[POINTS] =
	{
		{OFFSET_X, OFFSET_Y},
		{m_Screen.GetWidth ()-OFFSET_X, OFFSET_Y},
		{OFFSET_X, m_Screen.GetHeight ()-OFFSET_Y},
		{m_Screen.GetWidth ()-OFFSET_X, m_Screen.GetHeight ()-OFFSET_Y}
	};

	for (boolean bContinue = TRUE; bContinue;)
	{
		static const char ClearScreen[] = "\x1b[H\x1b[J\x1b[?25l";

		for (unsigned i = 0; i < POINTS; i++)
		{
			Print (ClearScreen);

			Print ("Please touch the center of red cross (step %u of 4)!", i+1);

			DrawLine (Point[i].nScreenX, Point[i].nScreenY-20,
				  Point[i].nScreenX, Point[i].nScreenY+20, HIGH_COLOR);
			DrawLine (Point[i].nScreenX-20, Point[i].nScreenY,
				  Point[i].nScreenX+20, Point[i].nScreenY, HIGH_COLOR);

			TPosition Pos = GetTouchPosition ();
			Point[i].nTouchX = Pos.x;
			Point[i].nTouchY = Pos.y;
		}

		Print (ClearScreen);

		if (   abs (Point[0].nTouchX - Point[2].nTouchX) > 100
		    || abs (Point[1].nTouchX - Point[3].nTouchX) > 100
		    || abs (Point[0].nTouchY - Point[1].nTouchY) > 100
		    || abs (Point[2].nTouchY - Point[3].nTouchY) > 100
		    || Point[0].nTouchX >= Point[1].nTouchX
		    || Point[0].nTouchY >= Point[2].nTouchY)
		{
			Print ("Invalid calibration data. Try again!\n");

			m_Timer.MsDelay (3000);
		}
		else
		{
			bContinue = FALSE;
		}
	}

	unsigned nScreenDiffX = Point[1].nScreenX - Point[0].nScreenX;
	unsigned nScreenDiffY = Point[2].nScreenY - Point[0].nScreenY;

	unsigned nTouchX1 = (Point[0].nTouchX + Point[2].nTouchX) / 2;
	unsigned nTouchX2 = (Point[1].nTouchX + Point[3].nTouchX) / 2;
	unsigned nTouchY1 = (Point[0].nTouchY + Point[1].nTouchY) / 2;
	unsigned nTouchY2 = (Point[2].nTouchY + Point[3].nTouchY) / 2;
	unsigned nTouchDiffX = nTouchX2 - nTouchX1;
	unsigned nTouchDiffY = nTouchY2 - nTouchY1;

	unsigned nScaleX = 1000*nTouchDiffX / nScreenDiffX;
	unsigned nScaleY = 1000*nTouchDiffY / nScreenDiffY;

	int nMinX = nTouchX1 - OFFSET_X*nScaleX/1000;	if (nMinX < 0) nMinX = 0;
	int nMinY = nTouchY1 - OFFSET_Y*nScaleY/1000;	if (nMinY < 0) nMinY = 0;
	int nMaxX = nTouchX2 + OFFSET_X*nScaleX/1000;
	int nMaxY = nTouchY2 + OFFSET_Y*nScaleY/1000;

	if (   nMinX < 20
	    && nMinY < 20
	    && abs (nMaxX - m_Screen.GetWidth ()) < 20
	    && abs (nMaxY - m_Screen.GetHeight ()) < 20)
	{
		Print ("This touchscreen does not need calibration.\n");
	}
	else
	{
		Print ("Add the following option to the first line of the file cmdline.txt:\n\n");

		Print ("touchscreen=%d,%d,%d,%d\n", nMinX, nMaxX, nMinY, nMaxY);
	}

	return ShutdownHalt;
}

CKernel::TPosition CKernel::GetTouchPosition (void)
{
	m_Timer.MsDelay (500);		// debouncing

	for (m_bTouched = FALSE; !m_bTouched;)
	{
		m_pTouchScreen->Update ();
	}

	return m_TouchPosition;
}

void CKernel::TouchScreenEventHandler (TTouchScreenEvent Event,
				       unsigned nID, unsigned nPosX, unsigned nPosY)
{
	assert (s_pThis != 0);

	if (   Event == TouchScreenEventFingerDown
	    && !s_pThis->m_bTouched)
	{
		s_pThis->m_TouchPosition.x = nPosX;
		s_pThis->m_TouchPosition.y = nPosY;
		s_pThis->m_bTouched = TRUE;
	}
}

void CKernel::Print (const char *pFormat, ...)
{
	CString Msg;

	va_list var;
	va_start (var, pFormat);

	Msg.FormatV (pFormat, var);

	va_end (var);

	m_Screen.Write ((const char *) Msg, Msg.GetLength ());
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

int CKernel::abs (int nValue)
{
	if (nValue < 0)
	{
		nValue = -nValue;
	}

	return nValue;
}
