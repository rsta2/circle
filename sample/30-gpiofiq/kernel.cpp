//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2019  R. Stange <rsta2@o2online.de>
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

#define GPIO_INPUT_PIN		17			// connect this pin to GPIO4 and oscilloscope
#define GPIO_OUTPUT_PIN		18			// connect this pin to oscilloscope

#define CLOCK_RATE		500000			// Hz

#define TRACE_DEPTH		20			// number of trace entries

#define RUNTIME_SECS		10

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_Clock0 (GPIOClock0),
	m_ClockPin (4, GPIOModeAlternateFunction0),
	m_InputPin (GPIO_INPUT_PIN, GPIOModeInput, &m_Interrupt),
	m_OutputPin (GPIO_OUTPUT_PIN, GPIOModeOutput),
	m_Tracer (TRACE_DEPTH, FALSE)
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

	if (!m_Clock0.StartRate (CLOCK_RATE))
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot generate %u Hz clock", CLOCK_RATE);
	}

	m_Logger.Write (FromKernel, LogNotice, "Running GPIO event capture for %u seconds", RUNTIME_SECS);

	m_Tracer.Start ();

	m_InputPin.ConnectInterrupt (FIQHandler, this);
	m_InputPin.EnableInterrupt (GPIOInterruptOnRisingEdge);
	m_InputPin.EnableInterrupt2 (GPIOInterruptOnFallingEdge);

	while (m_Timer.GetUptime () < RUNTIME_SECS)
	{
		// just wait
	}

	m_InputPin.DisableInterrupt ();
	m_InputPin.DisableInterrupt2 ();

	m_Tracer.Dump ();

	return ShutdownHalt;
}

void CKernel::FIQHandler (void *pParam)
{
	CKernel *pThis = (CKernel *) pParam;

	pThis->m_OutputPin.Invert ();

	pThis->m_Tracer.Event (1, pThis->m_InputPin.Read ());
}
