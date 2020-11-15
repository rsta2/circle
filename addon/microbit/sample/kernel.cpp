//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020  R. Stange <rsta2@o2online.de>
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

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer)
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
		bOK = m_Microbit.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	// scroll message on display
	if (m_Microbit.Scroll ("micro:bit meets Pi") < 0)
	{
		m_Logger.Write (FromKernel, LogPanic, "Scroll failed");
	}

	m_Timer.MsDelay (14000);	// wait for completion

	// set all pixels on display
	for (unsigned y = MICROBIT_POS_MIN; y <= MICROBIT_POS_MAX; y++)
	{
		for (unsigned x = MICROBIT_POS_MIN; x <= MICROBIT_POS_MAX; x++)
		{
			if (m_Microbit.SetPixel (x, y, MICROBIT_BRIGHT_MAX/2) < 0)
			{
				m_Logger.Write (FromKernel, LogPanic, "Set pixel failed");
			}

			m_Timer.MsDelay (200);
		}
	}

	// clear display
	if (m_Microbit.ClearDisplay () < 0)
	{
		m_Logger.Write (FromKernel, LogPanic, "Clear display failed");
	}

	// log current gesture and light level (until button A is pressed)
	CString Gesture;
	do
	{
		if (m_Microbit.GetCurrentGesture (&Gesture) < 0)
		{
			m_Logger.Write (FromKernel, LogPanic, "Get current gesture failed");
		}

		m_Logger.Write (FromKernel, LogNotice, "Current gesture is \"%s\"",
				(const char *) Gesture);

		m_Logger.Write (FromKernel, LogNotice, "Light level is %d",
				m_Microbit.GetLightLevel ());

		m_Timer.MsDelay (100);
	}
	while (!m_Microbit.IsPressed (MICROBIT_BUTTON_A));

	// calibrate compass and log heading
	if (m_Microbit.CalibrateCompass () < 0)
	{
		m_Logger.Write (FromKernel, LogPanic, "Calibrate compass failed");
	}

	while (1)
	{
		m_Logger.Write (FromKernel, LogNotice, "Heading is %d", m_Microbit.GetHeading ());

		m_Timer.MsDelay (100);
	}

	return ShutdownHalt;
}
