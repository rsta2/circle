//
// kernel.cpp
//
// Sample program for WS28XX controlled LED stripes
// Original development by Arjan van Vught <info@raspberrypi-dmx.nl>
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2020  R. Stange <rsta2@o2online.de>
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

#define SPI_MASTER_DEVICE	0		// 0, 4, 5, 6 on Raspberry Pi 4; 0 otherwise

#define WS28XX_TYPE		WS2801		// WS2801, WS2812, WS2812B or SK6812

#define LED_COUNT		170		// number of LEDs on the stripe

#define WS2801_SPI_SPEED	4000000		// Hz, only for WS2801, otherwise ignored

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_LEDStripe (WS28XX_TYPE, LED_COUNT, WS2801_SPI_SPEED, SPI_MASTER_DEVICE)
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
		bOK = m_LEDStripe.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	m_Logger.Write (FromKernel, LogNotice, "Sending RGB data to WS28XX LED stripe");

	for (;;)
	{
		u8 RGBColour[3] = {255, 0, 0};

		for (unsigned nDecColour = 0; nDecColour < 3; nDecColour++)
		{
			unsigned nIncColour = nDecColour == 2 ? 0 : nDecColour + 1;

			for (unsigned i = 0; i < 255; i++)
			{
				RGBColour[nDecColour] -= 1;
				RGBColour[nIncColour] += 1;

				for (int j = 0; j < LED_COUNT; j++)
				{
					m_LEDStripe.SetLED (j, RGBColour[0], RGBColour[1], RGBColour[2]);
				}

				CString Message;
				Message.Format ("\rR:%03u G:%03u B:%03u",
						(unsigned) RGBColour[0],
						(unsigned) RGBColour[1],
						(unsigned) RGBColour[2]);
				m_Screen.Write (Message, Message.GetLength ());

				if (!m_LEDStripe.Update ())
				{
					CLogger::Get ()->Write (FromKernel, LogPanic, "LED update failed");
				}

				m_Timer.MsDelay (5);
			}
		}
	}

	return ShutdownHalt;
}
