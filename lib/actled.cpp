//
// actled.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2019  R. Stange <rsta2@o2online.de>
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
#include <circle/actled.h>
#include <circle/machineinfo.h>
#include <circle/timer.h>

CActLED *CActLED::s_pThis = 0;

CActLED::CActLED (boolean bSafeMode)
:	m_pPin (0),
	m_pVirtualPin (0)
{
	s_pThis = this;

	unsigned nActLEDInfo = CMachineInfo::Get ()->GetActLEDInfo ();

	if (nActLEDInfo & ACTLED_VIRTUAL_PIN)
	{
		m_pVirtualPin = new CVirtualGPIOPin (nActLEDInfo & ACTLED_PIN_MASK, bSafeMode);
	}
	else
	{
		m_pPin = new CGPIOPin (nActLEDInfo & ACTLED_PIN_MASK, GPIOModeOutput);
	}

	m_bActiveHigh = nActLEDInfo & ACTLED_ACTIVE_LOW ? FALSE : TRUE;

	Off ();
}

CActLED::~CActLED (void)
{
	s_pThis = 0;
}

void CActLED::On (void)
{
	if (m_pPin != 0)
	{
		m_pPin->Write (m_bActiveHigh ? HIGH : LOW);
	}
	else if (m_pVirtualPin != 0)
	{
		m_pVirtualPin->Write (m_bActiveHigh ? HIGH : LOW);
	}
}

void CActLED::Off (void)
{
	if (m_pPin != 0)
	{
		m_pPin->Write (m_bActiveHigh ? LOW : HIGH);
	}
	else if (m_pVirtualPin != 0)
	{
		m_pVirtualPin->Write (m_bActiveHigh ? LOW : HIGH);
	}
}

void CActLED::Blink (unsigned nCount, unsigned nTimeOnMs, unsigned nTimeOffMs)
{
	for (unsigned i = 1; i <= nCount; i++)
	{
		On ();
		CTimer::SimpleMsDelay (nTimeOnMs);

		Off ();
		CTimer::SimpleMsDelay (nTimeOffMs);
	}
}

CActLED *CActLED::Get (void)
{
	return s_pThis;
}
