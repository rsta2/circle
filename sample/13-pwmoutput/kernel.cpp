//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#include <circle/types.h>
#include <assert.h>

#define PWM_RANGE	1024

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_PWMPin1 (18, GPIOModeAlternateFunction5),
	m_PWMPin2 (19, GPIOModeAlternateFunction5),
	m_PWMOutput (GPIOClockSourceOscillator, 2, PWM_RANGE, TRUE)
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

	m_PWMOutput.Start ();

	while (1)
	{
		for (unsigned nValue = 0; nValue <= PWM_RANGE; nValue++)
		{
			m_PWMOutput.Write (PWM_CHANNEL1, nValue);
			m_PWMOutput.Write (PWM_CHANNEL2, PWM_RANGE-nValue);

			m_Timer.MsDelay (4);
		}

		for (unsigned nValue = 0; nValue <= PWM_RANGE; nValue++)
		{
			m_PWMOutput.Write (PWM_CHANNEL1, PWM_RANGE-nValue);
			m_PWMOutput.Write (PWM_CHANNEL2, nValue);

			m_Timer.MsDelay (4);
		}

		m_Timer.MsDelay (1000);
	}

	return ShutdownHalt;
}
