//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
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
#include "scopewindow.h"
#include "controlwindow.h"

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (800, 480),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_Recorder (8, 9, 10, 11, &m_Config),
	m_DWHCI (&m_Interrupt, &m_Timer),
	m_Clock0 (GPIOClock0, GPIOClockSourceOscillator),	// clock source OSC, 19.2 MHz
	m_ClockPin (4, GPIOModeAlternateFunction0),
	m_GUI (&m_Screen)
{
	m_Clock0.Start (192);		// 19.2 MHz / 192 = 100 KHz
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
		bOK = m_Recorder.Initialize ();
	}

	if (bOK)
	{
		bOK = m_DWHCI.Initialize ();
	}

	if (bOK)
	{
		m_TouchScreen.Initialize ();

		bOK = m_GUI.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	CScopeWindow ScopeWindow (0, 0, &m_Recorder, &m_Config);
	CControlWindow ControlWindow (600, 0, &ScopeWindow, &m_Recorder, &m_Config);

	unsigned nLastTicks = m_Timer.GetClockTicks ();

	while (1)
	{
		m_GUI.Update ();

		unsigned nTicks = m_Timer.GetClockTicks ();
		if (nTicks - nLastTicks >= 4*CLOCKHZ)
		{
			m_CPUThrottle.SetOnTemperature ();	// call this every 4 seconds

			nLastTicks = nTicks;
		}
	}

	return ShutdownHalt;
}
