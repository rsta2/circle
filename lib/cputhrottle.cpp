//
// cputhrottle.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2022  R. Stange <rsta2@o2online.de>
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
#include <circle/cputhrottle.h>
#include <circle/bcmpropertytags.h>
#include <circle/timer.h>
#include <circle/koptions.h>
#include <circle/logger.h>
#include <assert.h>

#define TRANSITION_DELAY_USECS		355	// See: linux/drivers/cpufreq/bcm2835-cpufreq.c

static const char FromCPUThrottle[] = "throttle";

CCPUThrottle *CCPUThrottle::s_pThis = 0;

CCPUThrottle::CCPUThrottle (TCPUSpeed InitialSpeed)
:	m_bDynamic (FALSE),
	m_nMinClockRate (600),
	m_nMaxClockRate (600),
	m_nMaxTemperature (85000),
	m_nEnforcedTemperature (60000),
	m_SpeedSet (CPUSpeedUnknown),
	m_nTicksLastSet (0),
	m_nTicksLastUpdate (0),
	m_ThrottledStateMask (SystemStateNothingOccurred),
	m_LastThrottledState (SystemStateNothingOccurred),
	m_pThrottledHandler (0),
	m_pThrottledParam (0),
	m_bFanConnected (FALSE)
{
	assert (s_pThis == 0);
	s_pThis = this;
	assert (s_pThis != 0);

	unsigned nFanPin = CKernelOptions::Get ()->GetGPIOFanPin ();
	if (nFanPin > 0)
	{
		m_bFanConnected = TRUE;

		m_FanPin.AssignPin (nFanPin);
		m_FanPin.SetMode (GPIOModeOutput, FALSE);
		m_FanPin.Write (HIGH);
	}

	if (InitialSpeed == CPUSpeedUnknown)
	{
		InitialSpeed = CKernelOptions::Get ()->GetCPUSpeed ();
		assert (InitialSpeed < CPUSpeedUnknown);
	}

	m_nEnforcedTemperature = CKernelOptions::Get ()->GetSoCMaxTemp () * 1000;

	m_nMinClockRate = GetClockRate (PROPTAG_GET_MIN_CLOCK_RATE);
	if (m_nMinClockRate == 0)
	{
		return;
	}

	m_nMaxClockRate = GetClockRate (PROPTAG_GET_MAX_CLOCK_RATE);
	if (m_nMaxClockRate == 0)
	{
		return;
	}

	m_nMaxTemperature = GetTemperature (PROPTAG_GET_MAX_TEMPERATURE);
	if (m_nMaxTemperature == 0)
	{
		return;
	}

	assert (m_nEnforcedTemperature <= m_nMaxTemperature);

	if (m_nMinClockRate >= m_nMaxClockRate)
	{
		return;
	}

	m_bDynamic = TRUE;

	SetSpeed (InitialSpeed, TRUE);
}

CCPUThrottle::~CCPUThrottle (void)
{
	m_pThrottledHandler = 0;

	if (m_bFanConnected)
	{
		m_FanPin.SetMode (GPIOModeInput);
	}

	if (m_bDynamic)
	{
		SetSpeed (CPUSpeedLow, TRUE);

		m_bDynamic = FALSE;
	}

	s_pThis = 0;
}

boolean CCPUThrottle::IsDynamic (void) const
{
	return m_bDynamic;
}

unsigned CCPUThrottle::GetClockRate (void) const
{
	return GetClockRate (PROPTAG_GET_CLOCK_RATE);
}

unsigned CCPUThrottle::GetMinClockRate (void) const
{
	return m_nMinClockRate;
}

unsigned CCPUThrottle::GetMaxClockRate (void) const
{
	return m_nMaxClockRate;
}

unsigned CCPUThrottle::GetTemperature (void) const
{
	return (GetTemperature (PROPTAG_GET_TEMPERATURE) + 500) / 1000;
}

unsigned CCPUThrottle::GetMaxTemperature (void) const
{
	return (m_nMaxTemperature + 500) / 1000;
}

TCPUSpeed CCPUThrottle::SetSpeed (TCPUSpeed Speed, boolean bWait)
{
	if (!m_bDynamic)
	{
		return CPUSpeedMaximum;
	}

	if (!SetSpeedInternal (Speed, bWait))
	{
		return CPUSpeedUnknown;
	}

	TCPUSpeed PreviousSpeed = m_SpeedSet;
	m_SpeedSet = Speed;

	return PreviousSpeed;
}

boolean CCPUThrottle::SetOnTemperature (void)
{
	if (m_bFanConnected)
	{
		unsigned nTemperature = GetTemperature (PROPTAG_GET_TEMPERATURE);
		if (nTemperature == 0)
		{
			return FALSE;
		}

		assert (40000 <= m_nEnforcedTemperature);
		if (m_nEnforcedTemperature > m_nMaxTemperature)
		{
			m_nEnforcedTemperature = m_nMaxTemperature;
		}

		if (nTemperature > m_nEnforcedTemperature)
		{
			m_FanPin.Write (HIGH);
		}
		else if (nTemperature < (m_nEnforcedTemperature-5000))	// 5 degrees hysteresis
		{
			m_FanPin.Write (LOW);
		}

		return TRUE;
	}

	if (!m_bDynamic)
	{
		return TRUE;
	}

	unsigned nCurrentRate = GetClockRate (PROPTAG_GET_CLOCK_RATE);
	if (nCurrentRate == 0)
	{
		return FALSE;
	}

	unsigned nTemperature = GetTemperature (PROPTAG_GET_TEMPERATURE);
	if (nTemperature == 0)
	{
		return FALSE;
	}

	assert (40000 <= m_nEnforcedTemperature);
	if (m_nEnforcedTemperature > m_nMaxTemperature)
	{
		m_nEnforcedTemperature = m_nMaxTemperature;
	}

	if (   nTemperature > m_nEnforcedTemperature
	    && nCurrentRate > m_nMinClockRate)
	{
		if (!SetSpeedInternal (CPUSpeedLow, FALSE))
		{
			return FALSE;
		}
	}
	else if (   nTemperature < (m_nEnforcedTemperature-3000)	// 3 degrees hysteresis
		 && nCurrentRate < m_nMaxClockRate
		 && m_SpeedSet == CPUSpeedMaximum)
	{
		if (!SetSpeedInternal (m_SpeedSet, FALSE))
		{
			return FALSE;
		}
	}

	return TRUE;
}

boolean CCPUThrottle::Update (void)
{
	boolean bOK = TRUE;

	unsigned nTicks = CTimer::GetClockTicks ();
	if (nTicks - m_nTicksLastUpdate >= 4*CLOCKHZ)			// call this every 4 seconds
	{
		bOK = SetOnTemperature ();

		if (m_pThrottledHandler != 0)
		{
			bOK = CheckThrottledState () && bOK;
		}

		m_nTicksLastUpdate = nTicks;
	}

	return bOK;
}

void CCPUThrottle::RegisterSystemThrottledHandler (unsigned StateMask,
						   TSystemThrottledHandler *pHandler, void *pParam)
{
	assert (StateMask != 0);
	m_ThrottledStateMask = (TSystemThrottledState) StateMask;

	assert (m_pThrottledHandler == 0);
	m_pThrottledHandler = pHandler;
	assert (m_pThrottledHandler != 0);

	m_pThrottledParam = pParam;
}

void CCPUThrottle::DumpStatus (boolean bAll)
{
	CLogger *pLogger = CLogger::Get ();
	assert (pLogger != 0);

	if (bAll)
	{
		if (m_bDynamic)
		{
			pLogger->Write (FromCPUThrottle, LogDebug, "Clock rate range is %u-%u MHz",
					m_nMinClockRate / 1000000, m_nMaxClockRate / 1000000);

			pLogger->Write (FromCPUThrottle, LogDebug, "Maximum SoC temperature is %uC",
					m_nMaxTemperature / 1000);

			pLogger->Write (FromCPUThrottle, LogDebug, "Enforced SoC temperature is %uC",
					m_nEnforcedTemperature / 1000);
		}
		else
		{
			pLogger->Write (FromCPUThrottle, LogDebug, "Dynamic clock rate disabled");
		}
	}

	pLogger->Write (FromCPUThrottle, LogDebug, "Current clock rate is %u MHz",
			GetClockRate () / 1000000);

	pLogger->Write (FromCPUThrottle, LogDebug, "Current temperature is %uC", GetTemperature ());
}

CCPUThrottle *CCPUThrottle::Get (void)
{
	assert (s_pThis != 0);
	return s_pThis;
}

boolean CCPUThrottle::SetSpeedInternal (TCPUSpeed Speed, boolean bWait)
{
	assert (m_bDynamic);

	SetToSetDelay ();

	unsigned nClockRate;
	boolean bSkipTurbo;

	switch (Speed)
	{
	case CPUSpeedLow:
		nClockRate = m_nMinClockRate;
		bSkipTurbo = FALSE;
		break;

	case CPUSpeedMaximum:
		nClockRate = m_nMaxClockRate;
		bSkipTurbo = FALSE;
		break;

	default:
		assert (0);
		return FALSE;

	}

	if (!SetClockRate (nClockRate, bSkipTurbo))
	{
		return FALSE;
	}

	if (bWait)
	{
		CTimer::SimpleusDelay (TRANSITION_DELAY_USECS);
	}

	return TRUE;
}

boolean CCPUThrottle::CheckThrottledState (void)
{
	CBcmPropertyTags Tags;
	TPropertyTagSimple TagGetThrottled;
	TagGetThrottled.nValue = 0xFFFF;
	if (!Tags.GetTag (PROPTAG_GET_THROTTLED, &TagGetThrottled, sizeof TagGetThrottled, 4))
	{
		return FALSE;
	}

	TagGetThrottled.nValue &= m_ThrottledStateMask;

	if (   TagGetThrottled.nValue != 0
	    && TagGetThrottled.nValue != m_LastThrottledState)
	{
		assert (m_pThrottledHandler != 0);
		(*m_pThrottledHandler) ((TSystemThrottledState) TagGetThrottled.nValue,
					m_pThrottledParam);
	}

	m_LastThrottledState = (TSystemThrottledState) TagGetThrottled.nValue;

	return TRUE;
}

void CCPUThrottle::SetToSetDelay (void)
{
	unsigned nUsecElapsed = (CTimer::GetClockTicks () - m_nTicksLastSet) * (CLOCKHZ / 1000000);

	if (nUsecElapsed < TRANSITION_DELAY_USECS)
	{
		CTimer::SimpleusDelay (TRANSITION_DELAY_USECS - nUsecElapsed);
	}

	m_nTicksLastSet = CTimer::GetClockTicks ();
}

unsigned CCPUThrottle::GetClockRate (unsigned nTagId)
{
	CBcmPropertyTags Tags;
	TPropertyTagClockRate TagClockRate;
	TagClockRate.nClockId = CLOCK_ID_ARM;
	if (!Tags.GetTag (nTagId, &TagClockRate, sizeof TagClockRate, 4))
	{
		return 0;
	}

	return TagClockRate.nRate;
}

unsigned CCPUThrottle::GetTemperature (unsigned nTagId)
{
	CBcmPropertyTags Tags;
	TPropertyTagTemperature TagTemperature;
	TagTemperature.nTemperatureId = TEMPERATURE_ID;
	if (!Tags.GetTag (nTagId, &TagTemperature, sizeof TagTemperature, 4))
	{
		return 0;
	}

	return TagTemperature.nValue;
}

boolean CCPUThrottle::SetClockRate (unsigned nRate, boolean bSkipTurbo)
{
	CBcmPropertyTags Tags;
	TPropertyTagSetClockRate TagSetClockRate;
	TagSetClockRate.nClockId = CLOCK_ID_ARM;
	TagSetClockRate.nRate = nRate;
	TagSetClockRate.nSkipSettingTurbo = bSkipTurbo ? SKIP_SETTING_TURBO : 0;
	return Tags.GetTag (PROPTAG_SET_CLOCK_RATE, &TagSetClockRate, sizeof TagSetClockRate, 12);
}
