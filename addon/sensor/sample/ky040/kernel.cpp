//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2022  R. Stange <rsta2@o2online.de>
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
#include <circle/util.h>
#include <assert.h>

#define KY040_CLK_PIN	13
#define KY040_DT_PIN	19
#define KY040_SW_PIN	26

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_GPIOManager (&m_Interrupt),
	m_RotaryEncoder (&m_GPIOManager, KY040_CLK_PIN, KY040_DT_PIN, KY040_SW_PIN),
	m_nCount (0)
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
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	m_Logger.Write (FromKernel, LogNotice, "Just use your rotary encoder!");

	m_RotaryEncoder.RegisterEventHandler (EventHandler, this);

	for (unsigned nCount = 0; 1; nCount++)
	{
		m_Screen.Rotor (0, nCount);
	}

	return ShutdownHalt;
}

void CKernel::EventHandler (CKY040::TEvent Event, void *pParam)
{
	CKernel *pThis = static_cast<CKernel *> (pParam);
	assert (pThis != 0);

	const char *pMsg;

	switch (Event)
	{
	case CKY040::EventClockwise:
		pMsg = "CW  ";
		pThis->m_nCount++;
		pThis->m_nCount &= 3;
		break;

	case CKY040::EventCounterclockwise:
		pMsg = "CCW ";
		if (pThis->m_nCount)
		{
			pThis->m_nCount--;
		}
		else
		{
			pThis->m_nCount = 3;
		}
		break;

	case CKY040::EventSwitchDown:
		pMsg = "DWN ";
		pThis->m_ActLED.On ();
		break;

	case CKY040::EventSwitchUp:
		pMsg = "UP  ";
		pThis->m_ActLED.Off ();
		break;

	default:
		assert (0);
	}

	pThis->m_Screen.Write (pMsg, strlen (pMsg));

	pThis->m_Screen.Rotor (1, pThis->m_nCount);
}
