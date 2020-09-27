//
// latencytester.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2020  R. Stange <rsta2@o2online.de>
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
#include <circle/latencytester.h>
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <circle/debug.h>
#include <assert.h>

CLatencyTester::CLatencyTester (CInterruptSystem *pInterruptSystem)
:	m_pInterruptSystem (pInterruptSystem),
	m_bRunning (FALSE)
{
}

CLatencyTester::~CLatencyTester (void)
{
	if (m_bRunning)
	{
		Stop ();
	}
}

void CLatencyTester::Start (unsigned nSampleRateHZ)
{
	assert (!m_bRunning);

	m_nWantedDelay = (1000000 + nSampleRateHZ/2) / nSampleRateHZ;

	m_nMinDelay = (unsigned) -1;
	m_nMaxDelay = 0;
	m_nDelayAccu = 0;
	m_nSamples = 0;
	m_bOverflow = FALSE;

	m_bRunning = TRUE;

	PeripheralEntry ();

	write32 (ARM_SYSTIMER_C1, read32 (ARM_SYSTIMER_CLO) + m_nWantedDelay);
	
	PeripheralExit ();

	m_pInterruptSystem->ConnectIRQ (ARM_IRQ_TIMER1, InterruptStub, this);
}

void CLatencyTester::Stop (void)
{
	assert (m_bRunning);

	m_pInterruptSystem->DisconnectIRQ (ARM_IRQ_TIMER1);

	m_bRunning = FALSE;
}

unsigned CLatencyTester::GetMin (void) const
{
	return m_nMinDelay;
}

unsigned CLatencyTester::GetMax (void) const
{
	return m_nMaxDelay;
}

unsigned CLatencyTester::GetAvg (void)
{
	m_SpinLock.Acquire ();

	if (m_bOverflow)
	{
		m_SpinLock.Release ();

		return (unsigned) -1;
	}

	unsigned nDelayAccu = m_nDelayAccu;
	unsigned nSamples = m_nSamples;

	m_SpinLock.Release ();

	return nSamples > 0 ? nDelayAccu / nSamples : 0;
}

void CLatencyTester::Dump (void)
{
	CLogger::Get ()->Write ("latency", LogNotice, "IRQ latency: Min %u Max %u Avg %u (us)",
				GetMin (), GetMax (), GetAvg ());
}

void CLatencyTester::InterruptHandler (void)
{
	PeripheralEntry ();

	u32 nClock = read32 (ARM_SYSTIMER_CLO);
	u32 nCompare = read32 (ARM_SYSTIMER_C1);
	u32 nDelay = nClock - nCompare;

#ifndef NDEBUG
	//debug_click ();
#endif

	m_SpinLock.Acquire ();

	if (nDelay < m_nMinDelay)
	{
		m_nMinDelay = nDelay;
	}

	if (nDelay > m_nMaxDelay)
	{
		m_nMaxDelay = nDelay;
	}

	if (!m_bOverflow)
	{
		if (   m_nDelayAccu + nDelay >= m_nDelayAccu
		    && m_nSamples + 1 != 0)
		{
			m_nDelayAccu += nDelay;
			m_nSamples++;
		}
		else
		{
			m_bOverflow = TRUE;
		}
	}

	m_SpinLock.Release ();

	write32 (ARM_SYSTIMER_C1, read32 (ARM_SYSTIMER_CLO) + m_nWantedDelay);
	write32 (ARM_SYSTIMER_CS, 1 << 1);

	PeripheralExit ();
}

void CLatencyTester::InterruptStub (void *pParam)
{
	CLatencyTester *pTimer = (CLatencyTester *) pParam;

	pTimer->InterruptHandler ();
}
