//
// softpwmoutput.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018  R. Stange <rsta2@o2online.de>
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
#include "softpwmoutput.h"
#include <assert.h>

CSoftPWMOutput::CSoftPWMOutput (unsigned nGPIOPin, unsigned nRange)
:	m_GPIOPin (nGPIOPin, GPIOModeOutput),
	m_nRange (nRange),
	m_bRunning (FALSE)
{
	assert (m_nRange >= 1);
}

CSoftPWMOutput::~CSoftPWMOutput (void)
{
	assert (!m_bRunning);
}

void CSoftPWMOutput::Start (void)
{
	assert (!m_bRunning);

	m_nLevel = LOW;
	m_GPIOPin.Write (m_nLevel);

	m_nValue = m_nCurrentValue = 0;
	m_nCount = 1;

	m_bRunning = TRUE;
}

void CSoftPWMOutput::Stop (void)
{
	assert (m_bRunning);
	m_bRunning = FALSE;

	m_GPIOPin.Write (LOW);
}

void CSoftPWMOutput::Write (unsigned nValue)
{
	assert (m_bRunning);

	assert (nValue <= m_nRange);
	m_nValue = nValue;
}

void CSoftPWMOutput::ClockTick (void)
{
	if (m_bRunning)
	{
		unsigned nNewLevel = LOW;
		if (m_nCurrentValue > 0)
		{
			if (m_nCurrentValue < m_nRange)
			{
				nNewLevel = m_nCount <= m_nCurrentValue ? HIGH : LOW;
			}
			else
			{
				nNewLevel = HIGH;
			}
		}

		if (m_nLevel != nNewLevel)
		{
			m_nLevel = nNewLevel;
			m_GPIOPin.Write (nNewLevel);
		}

		if (++m_nCount > m_nRange)
		{
			m_nCurrentValue = m_nValue;
			m_nCount = 1;
		}
	}
}
