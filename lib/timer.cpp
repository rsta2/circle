//
// timer.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2015  R. Stange <rsta2@o2online.de>
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
#include <circle/timer.h>
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <circle/debug.h>
#include <assert.h>

extern "C" void DelayLoop (unsigned nCount);

const unsigned CTimer::s_nDaysOfMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

const char *CTimer::s_pMonthName[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

CTimer *CTimer::s_pThis = 0;

CTimer::CTimer (CInterruptSystem *pInterruptSystem)
:	m_pInterruptSystem (pInterruptSystem),
	m_nTicks (0),
	m_nTime (0),
	m_nMsDelay (350000),
	m_nusDelay (m_nMsDelay / 1000)
{
	assert (s_pThis == 0);
	s_pThis = this;

	for (unsigned hTimer = 0; hTimer < KERNEL_TIMERS; hTimer++)
	{
		m_KernelTimer[hTimer].m_pHandler = 0;
	}
}

CTimer::~CTimer (void)
{
	s_pThis = 0;
}

boolean CTimer::Initialize (void)
{
	assert (m_pInterruptSystem != 0);
	m_pInterruptSystem->ConnectIRQ (ARM_IRQ_TIMER3, InterruptHandler, this);

	DataMemBarrier ();

	write32 (ARM_SYSTIMER_CLO, -(30 * CLOCKHZ));	// timer wraps soon, to check for problems

	write32 (ARM_SYSTIMER_C3, read32 (ARM_SYSTIMER_CLO) + CLOCKHZ / HZ);
	
	TuneMsDelay ();

	DataMemBarrier ();

	return TRUE;
}

void CTimer::SetTime (unsigned nTime)
{
	m_TimeSpinLock.Acquire ();

	m_nTime = nTime;

	m_TimeSpinLock.Release ();
}

unsigned CTimer::GetClockTicks (void) const
{
	DataMemBarrier ();

	unsigned nResult = read32 (ARM_SYSTIMER_CLO);

	DataMemBarrier ();

	return nResult;
}

unsigned CTimer::GetTicks (void) const
{
	return m_nTicks;
}

unsigned CTimer::GetTime (void) const
{
	return m_nTime;
}

CString *CTimer::GetTimeString (void)
{
	m_TimeSpinLock.Acquire ();

	unsigned nTime = m_nTime;
	unsigned nTicks = m_nTicks;

	m_TimeSpinLock.Release ();

	if (   nTime == 0
	    && nTicks == 0)
	{
		return 0;
	}

	unsigned nSecond = nTime % 60;
	nTime /= 60;
	unsigned nMinute = nTime % 60;
	nTime /= 60;
	unsigned nHours = nTime;
	unsigned nHour = nTime % 24;
	nTime /= 24;

	unsigned nYear = 1970;
	while (1)
	{
		unsigned nDaysOfYear = IsLeapYear (nYear) ? 366 : 365;
		if (nTime < nDaysOfYear)
		{
			break;
		}

		nTime -= nDaysOfYear;
		nYear++;
	}

	unsigned nMonth = 0;
	while (1)
	{
		unsigned nDaysOfMonth = GetDaysOfMonth (nMonth, nYear);
		if (nTime < nDaysOfMonth)
		{
			break;
		}

		nTime -= nDaysOfMonth;
		nMonth++;
	}

	unsigned nMonthDay = nTime + 1;

	nTicks %= HZ;
#if (HZ != 100)
	nTicks = nTicks * 100 / HZ;
#endif

	CString *pString = new CString;
	assert (pString != 0);

	if (nYear > 1975)
	{
		pString->Format ("%s %2u %02u:%02u:%02u.%02u", s_pMonthName[nMonth], nMonthDay, nHour, nMinute, nSecond, nTicks);
	}
	else
	{
		pString->Format ("%02u:%02u:%02u.%02u", nHours, nMinute, nSecond, nTicks);
	}

	return pString;
}

unsigned CTimer::StartKernelTimer (unsigned nDelay,
				   TKernelTimerHandler *pHandler,
				   void *pParam,
				   void *pContext)
{
	m_KernelTimerSpinLock.Acquire ();

	unsigned hTimer;
	for (hTimer = 0; hTimer < KERNEL_TIMERS; hTimer++)
	{
		if (m_KernelTimer[hTimer].m_pHandler == 0)
		{
			break;
		}
	}

	if (hTimer >= KERNEL_TIMERS)
	{
		m_KernelTimerSpinLock.Release ();

		return 0;
	}

	assert (pHandler != 0);
	m_KernelTimer[hTimer].m_pHandler    = pHandler;
	m_KernelTimer[hTimer].m_nElapsesAt  = m_nTicks+nDelay;
	m_KernelTimer[hTimer].m_pParam      = pParam;
	m_KernelTimer[hTimer].m_pContext    = pContext;

	m_KernelTimerSpinLock.Release ();

	return hTimer+1;
}

void CTimer::CancelKernelTimer (unsigned hTimer)
{
	assert (1 <= hTimer && hTimer <= KERNEL_TIMERS);

	m_KernelTimerSpinLock.Acquire ();

	m_KernelTimer[hTimer-1].m_pHandler = 0;

	m_KernelTimerSpinLock.Release ();
}

void CTimer::PollKernelTimers (void)
{
	m_KernelTimerSpinLock.Acquire ();

	for (unsigned hTimer = 0; hTimer < KERNEL_TIMERS; hTimer++)
	{
		volatile TKernelTimer *pTimer = &m_KernelTimer[hTimer];

		TKernelTimerHandler *pHandler = pTimer->m_pHandler;
		if (pHandler != 0)
		{
			if ((int) (pTimer->m_nElapsesAt-m_nTicks) <= 0)
			{
				pTimer->m_pHandler = 0;

				void *pParam = pTimer->m_pParam;
				void *pContext = pTimer->m_pContext;

				m_KernelTimerSpinLock.Release ();

				(*pHandler) (hTimer+1, pParam, pContext);

				m_KernelTimerSpinLock.Acquire ();
			}
		}
	}

	m_KernelTimerSpinLock.Release ();
}

void CTimer::InterruptHandler (void)
{
	DataMemBarrier ();

	assert (read32 (ARM_SYSTIMER_CS) & (1 << 3));
	
	u32 nCompare = read32 (ARM_SYSTIMER_C3) + CLOCKHZ / HZ;
	write32 (ARM_SYSTIMER_C3, nCompare);
	if (nCompare < read32 (ARM_SYSTIMER_CLO))			// time may drift
	{
		nCompare = read32 (ARM_SYSTIMER_CLO) + CLOCKHZ / HZ;
		write32 (ARM_SYSTIMER_C3, nCompare);
	}

	write32 (ARM_SYSTIMER_CS, 1 << 3);

	DataMemBarrier ();

#ifndef NDEBUG
	//debug_click ();
#endif

	m_TimeSpinLock.Acquire ();

	if (++m_nTicks % HZ == 0)
	{
		m_nTime++;
	}

	m_TimeSpinLock.Release ();

	PollKernelTimers ();
}

void CTimer::InterruptHandler (void *pParam)
{
	CTimer *pThis = (CTimer *) pParam;
	assert (pThis != 0);

	pThis->InterruptHandler ();
}

void CTimer::MsDelay (unsigned nMilliSeconds)
{
	if (nMilliSeconds > 0)
	{
		unsigned nCycles =  m_nMsDelay * nMilliSeconds;

		DelayLoop (nCycles);
	}
}

void CTimer::usDelay (unsigned nMicroSeconds)
{
	if (nMicroSeconds > 0)
	{
		unsigned nCycles =  m_nusDelay * nMicroSeconds;

		DelayLoop (nCycles);
	}
}

void CTimer::TuneMsDelay (void)
{
	unsigned nTicks = GetTicks ();
	MsDelay (1000);
	nTicks = GetTicks () - nTicks;

	unsigned nFactor = 100 * HZ / nTicks;

	m_nMsDelay = m_nMsDelay * nFactor / 100;
	m_nusDelay = (m_nMsDelay + 500) / 1000;

	CLogger::Get ()->Write ("timer", LogNotice, "SpeedFactor is %u.%02u",
				nFactor / 100, nFactor % 100);
}

void CTimer::SimpleMsDelay (unsigned nMilliSeconds)
{
	if (nMilliSeconds > 0)
	{
		SimpleusDelay (nMilliSeconds * 1000);
	}
}

void CTimer::SimpleusDelay (unsigned nMicroSeconds)
{
	if (nMicroSeconds > 0)
	{
		unsigned nTicks = nMicroSeconds * (CLOCKHZ / 1000000);

		DataMemBarrier ();

		unsigned nStartTicks = read32 (ARM_SYSTIMER_CLO);
		while (read32 (ARM_SYSTIMER_CLO) - nStartTicks < nTicks)
		{
			// do nothing
		}

		DataMemBarrier ();
	}
}

int CTimer::IsLeapYear (unsigned nYear)
{
	if (nYear % 100 == 0)
	{
		return nYear % 400 == 0;
	}

	return nYear % 4 == 0;
}

unsigned CTimer::GetDaysOfMonth (unsigned nMonth, unsigned nYear)
{
	if (   nMonth == 1
	    && IsLeapYear (nYear))
	{
		return 29;
	}

	return s_nDaysOfMonth[nMonth];
}

CTimer *CTimer::Get (void)
{
	assert (s_pThis != 0);
	return s_pThis;
}
