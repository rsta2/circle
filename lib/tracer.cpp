//
// tracer.cpp
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
#include <circle/tracer.h>
#include <circle/timer.h>
#include <circle/logger.h>

static const char FromTracer[] = "trace";

CTracer *CTracer::s_pThis = 0;

CTracer::CTracer (unsigned nDepth, boolean bStopIfFull)
: 	m_nDepth (nDepth),
	m_bStopIfFull (bStopIfFull),
	m_bActive (FALSE),
	m_nStartTicks (0),
	m_nEntries (0),
	m_nCurrent (0)
{
	s_pThis = this;

	m_pEntry = new TTraceEntry[nDepth];
}

CTracer::~CTracer (void)
{
	delete [] m_pEntry;

	s_pThis = 0;
}

void CTracer::Start (void)
{
	m_nStartTicks = CTimer::Get ()->GetClockTicks ();

	m_bActive = TRUE;
}

void CTracer::Stop (void)
{
	Event (TRACER_EVENT_STOP);

	m_bActive = FALSE;
}

void CTracer::Event (unsigned nID, unsigned nParam1, unsigned nParam2, unsigned nParam3, unsigned nParam4)
{
	if (m_bActive)
	{
		if (m_nEntries < m_nDepth)
		{
			m_nEntries++;
		}
		else if (m_bStopIfFull)
		{
			m_bActive = FALSE;

			return;
		}

		TTraceEntry *pEntry = m_pEntry + m_nCurrent;

		pEntry->nClockTicks = CTimer::Get ()->GetClockTicks () - m_nStartTicks;
		pEntry->nEventID    = nID;
		pEntry->nParam[0]   = nParam1;
		pEntry->nParam[1]   = nParam2;
		pEntry->nParam[2]   = nParam3;
		pEntry->nParam[3]   = nParam4;

		if (++m_nCurrent == m_nDepth)
		{
			m_nCurrent = 0;
		}
	}
}

void CTracer::Dump (void)
{
	if (m_bActive)
	{
		Stop ();
	}
	
	CLogger *pLogger = CLogger::Get ();

	unsigned nEvent = 0;
	if (m_nEntries == m_nDepth)
	{
		nEvent = m_nCurrent;
	}
	
	for (unsigned i = 1; i <= m_nEntries; i++)
	{	
		TTraceEntry *pEntry = m_pEntry + nEvent;

		pLogger->Write (FromTracer, LogNotice, "%2u: %2u.%06u %2u %08X %08X %08X %08X",
				i, pEntry->nClockTicks / CLOCKHZ, pEntry->nClockTicks % CLOCKHZ,
				pEntry->nEventID, pEntry->nParam[0], pEntry->nParam[1], pEntry->nParam[2], pEntry->nParam[3]);

		nEvent = (nEvent+1) % m_nDepth;
	}
}

CTracer *CTracer::Get (void)
{
	return s_pThis;
}
