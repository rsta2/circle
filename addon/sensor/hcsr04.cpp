//
// hcsr04.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017  R. Stange <rsta2@o2online.de>
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
#include <sensor/hcsr04.h>
#include <circle/timer.h>
#include <circle/synchronize.h>

CHCSR04::CHCSR04 (unsigned nTriggerPin, unsigned nEchoPin)
:	m_TriggerPin (nTriggerPin, GPIOModeOutput),
	m_EchoPin (nEchoPin, GPIOModeInput),
	m_nDistance (0)
{
}

CHCSR04::~CHCSR04 (void)
{
}

boolean CHCSR04::Initialize (void)
{
	m_TriggerPin.Write (LOW);
	CTimer::Get ()->usDelay (10);

	return TRUE;
}

boolean CHCSR04::DoMeasurement (unsigned nTimeoutMicros)
{
	if (m_EchoPin.Read () == HIGH)
	{
		return FALSE;
	}

	EnterCritical ();

	m_TriggerPin.Write (HIGH);
	CTimer::Get ()->usDelay (10);
	m_TriggerPin.Write (LOW);

	LeaveCritical ();

	unsigned nStartTicks = CTimer::Get ()->GetClockTicks ();
	while (m_EchoPin.Read () == LOW)
	{
		if (CTimer::Get ()->GetClockTicks () - nStartTicks >= nTimeoutMicros)
		{
			return FALSE;
		}
	}

	unsigned nStartTicksHigh = CTimer::Get ()->GetClockTicks ();
	while (m_EchoPin.Read () == HIGH)
	{
		if (CTimer::Get ()->GetClockTicks () - nStartTicks >= nTimeoutMicros)
		{
			return FALSE;
		}
	}

	unsigned nInterval = CTimer::Get ()->GetClockTicks () - nStartTicksHigh;

	m_nDistance = nInterval * 10 / 58;

	return TRUE;
}

unsigned CHCSR04::GetDistance (void) const
{
	return m_nDistance;
}
