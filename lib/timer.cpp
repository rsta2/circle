//
// timer.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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

struct TKernelTimer
{
#ifndef NDEBUG
	unsigned	     m_nMagic;
#define KERNEL_TIMER_MAGIC	0x4B544D43
#endif
	TKernelTimerHandler *m_pHandler;
	unsigned	     m_nElapsesAt;
	void 		    *m_pParam;
	void 		    *m_pContext;
};

extern "C" void DelayLoop (unsigned nCount);

static const char FromTimer[] = "timer";

const unsigned CTimer::s_nDaysOfMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

const char *CTimer::s_pMonthName[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

CTimer *CTimer::s_pThis = 0;

CTimer::CTimer (CInterruptSystem *pInterruptSystem)
:	m_pInterruptSystem (pInterruptSystem),
	m_nTicks (0),
	m_nUptime (0),
	m_nTime (0),
	m_nMinutesDiff (0),
	m_nMsDelay (350000),
	m_nusDelay (m_nMsDelay / 1000)
{
	assert (s_pThis == 0);
	s_pThis = this;
}

CTimer::~CTimer (void)
{
	assert (m_pInterruptSystem != 0);
	m_pInterruptSystem->DisconnectIRQ (ARM_IRQ_TIMER3);

	TPtrListElement *pElement;
	while ((pElement = m_KernelTimerList.GetFirst ()) != 0)
	{
		TKernelTimer *pTimer = (TKernelTimer *) m_KernelTimerList.GetPtr (pElement);
		assert (pTimer != 0);
		assert (pTimer->m_nMagic == KERNEL_TIMER_MAGIC);

		m_KernelTimerList.Remove (pElement);

		delete pTimer;
	}

	s_pThis = 0;
}

boolean CTimer::Initialize (void)
{
	assert (m_pInterruptSystem != 0);
	m_pInterruptSystem->ConnectIRQ (ARM_IRQ_TIMER3, InterruptHandler, this);

	PeripheralEntry ();

	write32 (ARM_SYSTIMER_CLO, -(30 * CLOCKHZ));	// timer wraps soon, to check for problems

	write32 (ARM_SYSTIMER_C3, read32 (ARM_SYSTIMER_CLO) + CLOCKHZ / HZ);
	
	TuneMsDelay ();

	PeripheralExit ();

	return TRUE;
}

boolean CTimer::SetTimeZone (int nMinutesDiff)
{
	if (-24*60 < nMinutesDiff && nMinutesDiff < 24*60)
	{
		m_nMinutesDiff = nMinutesDiff;

		return TRUE;
	}

	return FALSE;
}

int CTimer::GetTimeZone (void) const
{
	return m_nMinutesDiff;
}

boolean CTimer::SetTime (unsigned nTime, boolean bLocal)
{
	if (!bLocal)
	{
		int nSecondsDiff = m_nMinutesDiff * 60;
		if (    nSecondsDiff < 0
		    && -nSecondsDiff > (int) nTime)
		{
			return FALSE;
		}

		nTime += nSecondsDiff;
	}

	m_TimeSpinLock.Acquire ();

	m_nTime = nTime;

	m_TimeSpinLock.Release ();

	return TRUE;
}

unsigned CTimer::GetClockTicks (void)
{
	PeripheralEntry ();

	unsigned nResult = read32 (ARM_SYSTIMER_CLO);

	PeripheralExit ();

	return nResult;
}

unsigned CTimer::GetTicks (void) const
{
	return m_nTicks;
}

unsigned CTimer::GetUptime (void) const
{
	return m_nUptime;
}

unsigned CTimer::GetTime (void) const
{
	return m_nTime;
}

unsigned CTimer::GetUniversalTime (void) const
{
	unsigned nResult = m_nTime;

	int nSecondsDiff = m_nMinutesDiff * 60;
	if (nSecondsDiff > (int) nResult)
	{
		return 0;
	}

	return nResult - nSecondsDiff;
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
	TKernelTimer *pTimer = new TKernelTimer;
	assert (pTimer != 0);

	unsigned nElapsesAt = m_nTicks + nDelay;

	assert (pHandler != 0);
#ifndef NDEBUG
	pTimer->m_nMagic     = KERNEL_TIMER_MAGIC;
#endif
	pTimer->m_pHandler   = pHandler;
	pTimer->m_nElapsesAt = nElapsesAt;
	pTimer->m_pParam     = pParam;
	pTimer->m_pContext   = pContext;

	m_KernelTimerSpinLock.Acquire ();

	TPtrListElement *pPrevElement = 0;
	TPtrListElement *pElement = m_KernelTimerList.GetFirst ();
	while (pElement != 0)
	{
		TKernelTimer *pTimer2 = (TKernelTimer *) m_KernelTimerList.GetPtr (pElement);
		assert (pTimer2 != 0);
		assert (pTimer2->m_nMagic == KERNEL_TIMER_MAGIC);

		if ((int) (pTimer2->m_nElapsesAt-nElapsesAt) > 0)
		{
			break;
		}

		pPrevElement = pElement;
		pElement = m_KernelTimerList.GetNext (pElement);
	}

	if (pElement != 0)
	{
		m_KernelTimerList.InsertBefore (pElement, pTimer);
	}
	else
	{
		m_KernelTimerList.InsertAfter (pPrevElement, pTimer);
	}

	m_KernelTimerSpinLock.Release ();

	return (unsigned) pTimer;
}

void CTimer::CancelKernelTimer (unsigned hTimer)
{
	TKernelTimer *pTimer = (TKernelTimer *) hTimer;
	assert (pTimer != 0);

	m_KernelTimerSpinLock.Acquire ();

	TPtrListElement *pElement = m_KernelTimerList.Find (pTimer);
	if (pElement != 0)
	{
		assert (pTimer->m_nMagic == KERNEL_TIMER_MAGIC);

		m_KernelTimerList.Remove (pElement);

#ifndef NDEBUG
		pTimer->m_nMagic = 0;
#endif
		delete pTimer;
	}

	m_KernelTimerSpinLock.Release ();
}

void CTimer::PollKernelTimers (void)
{
	m_KernelTimerSpinLock.Acquire ();

	TPtrListElement *pElement = m_KernelTimerList.GetFirst ();
	while (pElement != 0)
	{
		TKernelTimer *pTimer = (TKernelTimer *) m_KernelTimerList.GetPtr (pElement);
		assert (pTimer != 0);
		assert (pTimer->m_nMagic == KERNEL_TIMER_MAGIC);

		if ((int) (pTimer->m_nElapsesAt-m_nTicks) > 0)
		{
			break;
		}

		TPtrListElement *pNextElement = m_KernelTimerList.GetNext (pElement);
		m_KernelTimerList.Remove (pElement);
		pElement = pNextElement;

		m_KernelTimerSpinLock.Release ();

		TKernelTimerHandler *pHandler = pTimer->m_pHandler;
		assert (pHandler != 0);
		(*pHandler) ((unsigned) pTimer, pTimer->m_pParam, pTimer->m_pContext);

#ifndef NDEBUG
		pTimer->m_nMagic = 0;
#endif
		delete pTimer;

		m_KernelTimerSpinLock.Acquire ();
	}

	m_KernelTimerSpinLock.Release ();
}

void CTimer::InterruptHandler (void)
{
	PeripheralEntry ();

	assert (read32 (ARM_SYSTIMER_CS) & (1 << 3));
	
	u32 nCompare = read32 (ARM_SYSTIMER_C3) + CLOCKHZ / HZ;
	write32 (ARM_SYSTIMER_C3, nCompare);
	if (nCompare < read32 (ARM_SYSTIMER_CLO))			// time may drift
	{
		nCompare = read32 (ARM_SYSTIMER_CLO) + CLOCKHZ / HZ;
		write32 (ARM_SYSTIMER_C3, nCompare);
	}

	write32 (ARM_SYSTIMER_CS, 1 << 3);

	PeripheralExit ();

#ifndef NDEBUG
	//debug_click ();
#endif

	m_TimeSpinLock.Acquire ();

	if (++m_nTicks % HZ == 0)
	{
		m_nUptime++;
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

void CTimer::TuneMsDelay (void)
{
	unsigned nTicks = GetTicks ();
	DelayLoop (m_nMsDelay * 1000);
	nTicks = GetTicks () - nTicks;

	unsigned nFactor = 100 * HZ / nTicks;

	m_nMsDelay = m_nMsDelay * nFactor / 100;
	m_nusDelay = (m_nMsDelay + 500) / 1000;

	CLogger::Get ()->Write (FromTimer, LogNotice, "SpeedFactor is %u.%02u",
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
		unsigned nTicks = nMicroSeconds * (CLOCKHZ / 1000000) + 1;

		PeripheralEntry ();

		unsigned nStartTicks = read32 (ARM_SYSTIMER_CLO);
		while (read32 (ARM_SYSTIMER_CLO) - nStartTicks < nTicks)
		{
			// do nothing
		}

		PeripheralExit ();
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
