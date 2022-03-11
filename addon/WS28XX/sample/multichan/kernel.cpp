//
// kernel.cpp
//
// Sample program for WS2812 controlled LED stripes / NeoPixels (over SMI)
// Based on an example by Sebastien Nicolas <seba1978@gmx.de>
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2022  R. Stange <rsta2@o2online.de>
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

// GPIO pins, to which the three LED stripes are connected
#define GPIO_PIN1		8		// 8 .. 23
#define GPIO_PIN2		9		// 8 .. 23
#define GPIO_PIN3		10		// 8 .. 23

#define LED_COUNT		10		// number of LEDs on the stripes

#define INTENSITY		50		// 0 .. 255
#define DELAY_MILLIS		100


#define GPIO_TO_SD_LINE(pin)	((pin) - 8)
#define SD_LINE1		GPIO_TO_SD_LINE (GPIO_PIN1)
#define SD_LINE2		GPIO_TO_SD_LINE (GPIO_PIN2)
#define SD_LINE3		GPIO_TO_SD_LINE (GPIO_PIN3)

#define SD_LINE_TO_MASK(line)	(1 << (line))
#define SD_LINES_MASK		(  SD_LINE_TO_MASK (SD_LINE1) \
				 | SD_LINE_TO_MASK (SD_LINE2) \
				 | SD_LINE_TO_MASK (SD_LINE3))

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_NeoPixels (SD_LINES_MASK, LED_COUNT)
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

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	m_Logger.Write (FromKernel, LogNotice, "Sending RGB data to WS2812 NeoPixels");

	unsigned nLED = 0;
	while (1)
	{
		m_NeoPixels.SetLED (SD_LINE1, nLED, 0, 0, 0);
		m_NeoPixels.SetLED (SD_LINE2, nLED, 0, 0, 0);
		m_NeoPixels.SetLED (SD_LINE3, nLED, 0, 0, 0);

		if (++nLED >= LED_COUNT)
		{
			nLED = 0;
		}

		CString Msg;
		Msg.Format ("\rLED %3u", nLED);
		m_Screen.Write ((const char *) Msg, Msg.GetLength ());

		m_NeoPixels.SetLED (SD_LINE1, nLED, INTENSITY, 0, 0);	// red
		m_NeoPixels.SetLED (SD_LINE2, nLED, 0, INTENSITY, 0);	// green
		m_NeoPixels.SetLED (SD_LINE3, nLED, 0, 0, INTENSITY);	// blue

		m_NeoPixels.Update ();

		m_Timer.MsDelay (DELAY_MILLIS);
	}

	return ShutdownHalt;
}
