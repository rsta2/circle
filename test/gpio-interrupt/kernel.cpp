//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023  R. Stange <rsta2@o2online.de>
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

#define GPIO_BUTTON	17	// connect a button between this GPIO and GND

LOGMODULE ("kernel");

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_GPIOManager (&m_Interrupt),
	m_ButtonPin (GPIO_BUTTON, GPIOModeInputPullUp, &m_GPIOManager)
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
		bOK = m_GPIOManager.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	LOGNOTE ("Compile time: " __DATE__ " " __TIME__);

	m_ButtonPin.ConnectInterrupt (InterruptHandler, this);

	LOGNOTE ("Just press the button!");

	m_ButtonPin.EnableInterrupt (GPIOInterruptOnFallingEdge);
	LOGNOTE ("Mode is FallingEdge for 10 seconds");
	m_Timer.MsDelay (10000);
	m_ButtonPin.DisableInterrupt ();

	m_ButtonPin.EnableInterrupt (GPIOInterruptOnLowLevel);
	LOGNOTE ("Mode is LowLevel for 10 seconds");
	m_Timer.MsDelay (10000);
	m_ButtonPin.DisableInterrupt ();

#if RASPPI >= 5
	m_ButtonPin.EnableInterrupt (GPIOInterruptOnDebouncedLowLevel);
	LOGNOTE ("Mode is DebouncedLowLevel for 10 seconds");
	m_Timer.MsDelay (10000);
	m_ButtonPin.DisableInterrupt ();
#endif

	m_ButtonPin.DisconnectInterrupt ();

	LOGNOTE ("Program will halt now");

	return ShutdownHalt;
}

void CKernel::InterruptHandler (void *pParam)
{
	LOGNOTE  ("GPIO interrupt");
}
