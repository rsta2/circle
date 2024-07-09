//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2024  R. Stange <rsta2@o2online.de>
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
#include <circle/input/keyboardbuffer.h>
#include <circle/time.h>
#include <circle/string.h>
#include <circle/util.h>
#include <circle/machineinfo.h>

#define SPI_MASTER_DEVICE	0		// 0, 4, 5, 6 on Raspberry Pi 4; 0 otherwise
#define SPI_CLOCK_SPEED		15000000	// Hz
#define SPI_CPOL		1		// try 0, if it does not work
#define SPI_CPHA		0		// try 1, if it does not work
#define SPI_CHIP_SELECT		0		// 0 or 1; don't care, if not connected

#define WIDTH			240		// display width in pixels
#define HEIGHT			240		// display height in pixels
#define DC_PIN			22
#define RESET_PIN		23		// or CST7789Display::None
#define BACKLIGHT_PIN	CST7789Display::None

#define COLS  15	// Max 40
#define ROWS  4		// Max 10
#define ROT   0 	// 0,90,180,270
#define WIDECHARS	TRUE	// Set to false for thin characters
#define TALLCHARS	TRUE	// Set to false for short characters

#define MY_COLOR		ST7789_COLOR (31, 31, 15)	// any color

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer),
	m_SPIMaster (SPI_CLOCK_SPEED, SPI_CPOL, SPI_CPHA, SPI_MASTER_DEVICE),
	m_Display (&m_SPIMaster, DC_PIN, RESET_PIN, BACKLIGHT_PIN, WIDTH, HEIGHT,
		   SPI_CPOL, SPI_CPHA, SPI_CLOCK_SPEED, SPI_CHIP_SELECT),
	m_pLCD (nullptr)
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
		bOK = m_USBHCI.Initialize ();
	}
	
	if (bOK)
	{
		bOK = m_SPIMaster.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Display.Initialize ();
		m_Display.SetRotation(ROT);
	}

	if (bOK)
	{
		// Cannot instantiate the ST7789 device until the disply has been initialised
		m_pLCD = new CST7789Device (&m_SPIMaster, &m_Display, COLS, ROWS, WIDECHARS, TALLCHARS);
		assert (m_pLCD);
		bOK = m_pLCD->Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	LCDWrite ("01234567890123456789");
	LCDWrite (">>>>>>>>>>>>>>>>>>>");

	unsigned nTime = 10 + m_Timer.GetTime ();
	while (nTime > m_Timer.GetTime ())
	{
		// just wait a few seconds
	}
	
	LCDWrite ("\E[H\E[J"); // Reset cursor and clear display

	CUSBKeyboardDevice *pKeyboard =
		(CUSBKeyboardDevice *) m_DeviceNameService.GetDevice ("ukbd1", FALSE);
	if (pKeyboard == 0)
	{
		m_Logger.Write (FromKernel, LogError, "Keyboard not found");

		TimeDemo ();

		return ShutdownHalt;
	}

	CKeyboardBuffer Keyboard (pKeyboard);

	m_Logger.Write (FromKernel, LogNotice, "Just type something!");
	LCDWrite ("Just type!\n");

	for (unsigned nCount = 0; 1; nCount++)
	{
		char Buffer[10];
		int nResult = Keyboard.Read (Buffer, sizeof Buffer);
		if (nResult > 0)
		{
			m_pLCD->Write (Buffer, nResult);
		}

		m_Screen.Rotor (0, nCount);
	}

	return ShutdownHalt;
}

void CKernel::TimeDemo (void)
{
	LCDWrite ("\x1B[?25l");		// cursor off
	LCDWrite ("------------\n");
	LCDWrite ("No keyboard!\n");

	unsigned nTime = m_Timer.GetTime ();
	while (1)
	{
		while (nTime == m_Timer.GetTime ())
		{
			// just wait a second
		}

		nTime = m_Timer.GetTime ();

		CTime Time;
		Time.Set (nTime);

		CString String;
		String.Format ("\r%02u:%02u:%02u",
			Time.GetHours (), Time.GetMinutes (), Time.GetSeconds ());

		LCDWrite ((const char *) String);
	}
}

void CKernel::LCDWrite (const char *pString)
{
	m_pLCD->Write (pString, strlen (pString));
}
