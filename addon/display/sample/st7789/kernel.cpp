//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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

#define SPI_MASTER_DEVICE	0		// 0, 4, 5, 6 on Raspberry Pi 4; 0 otherwise
#define SPI_CLOCK_SPEED		15000000	// Hz
#define SPI_CPOL		1		// try 0, if it does not work
#define SPI_CPHA		0		// try 1, if it does not work
#define SPI_CHIP_SELECT		0		// 0 or 1; don't care, if not connected

#define WIDTH			240		// display width in pixels
#define HEIGHT			240		// display height in pixels
#define DC_PIN			22
#define RESET_PIN		23		// or: CST7789Display::None
#define BACKLIGHT_PIN		CST7789Display::None

#define MY_COLOR		ST7789_COLOR (31, 31, 15)	// any color

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_SPIMaster (SPI_CLOCK_SPEED, SPI_CPOL, SPI_CPHA, SPI_MASTER_DEVICE),
	m_Display (&m_SPIMaster, DC_PIN, RESET_PIN, BACKLIGHT_PIN, WIDTH, HEIGHT,
		   SPI_CPOL, SPI_CPHA, SPI_CLOCK_SPEED, SPI_CHIP_SELECT)
{
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
		bOK = m_SPIMaster.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Display.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

#if WIDTH >= 240 && HEIGHT >= 100
	// draw some text
	m_Display.DrawText (10, 10, "Hello display!", MY_COLOR);

	// countdown
	static const unsigned InitialCounter = 5000;
	for (int n = InitialCounter; n > 0; n--)
	{
		CString String;
		String.Format ("%4d", n);

		m_Display.DrawText (10, 50, String, MY_COLOR);

		unsigned nPosX = (WIDTH-1) * (InitialCounter-n) / InitialCounter;
		m_Display.SetPixel (nPosX, 98, MY_COLOR);
		m_Display.SetPixel (nPosX, 99, MY_COLOR);
	}

	m_Display.Clear ();
#endif

	// draw rectangle
	for (unsigned nPosX = 0; nPosX < WIDTH; nPosX++)
	{
		m_Display.SetPixel (nPosX, 0, MY_COLOR);
		m_Display.SetPixel (nPosX, WIDTH-1, MY_COLOR);
	}
	for (unsigned nPosY = 0; nPosY < WIDTH; nPosY++)
	{
		m_Display.SetPixel (0, nPosY, MY_COLOR);
		m_Display.SetPixel (WIDTH-1, nPosY, MY_COLOR);
	}

	// draw cross
	for (unsigned nPosX = 0; nPosX < WIDTH; nPosX++)
	{
		unsigned nPosY = nPosX * WIDTH / WIDTH;

		m_Display.SetPixel (nPosX, nPosY, MY_COLOR);
		m_Display.SetPixel (WIDTH-nPosX-1, nPosY, MY_COLOR);
	}

	m_Timer.MsDelay (5000);

	m_Display.Off ();

	return ShutdownReboot;
}
