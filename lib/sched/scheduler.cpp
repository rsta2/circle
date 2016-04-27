//
// scheduler.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>
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
#include <circle/sched/scheduler.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <assert.h>

static const char FromScheduler[] = "sched";

CScheduler *CScheduler::s_pThis = 0;

CScheduler::CScheduler (void)
:	m_nTasks (0),
	m_pCurrent (0),
	m_nCurrent (0)
{
	assert (s_pThis == 0);
	s_pThis = this;

	m_pCurrent = new CTask (0);		// main task currently running
	assert (m_pCurrent != 0);
}

CScheduler::~CScheduler (void)
{
	assert (m_nTasks == 1);
	assert (m_pTask[0] == m_pCurrent);
	RemoveTask (m_pCurrent);
	delete m_pCurrent;
	m_pCurrent = 0;

	s_pThis = 0;
}

void CScheduler::Yield (void)
{
	while ((m_nCurrent = GetNextTask ()) == MAX_TASKS)	// no task is ready
	{
		assert (m_nTasks > 0);
	}

	assert (m_nCurrent < MAX_TASKS);
	CTask *pNext = m_pTask[m_nCurrent];
	assert (pNext != 0);
	if (m_pCurrent == pNext)
	{
		return;
	}
	
	TTaskRegisters *pOldRegs = m_pCurrent->GetRegs ();
	m_pCurrent = pNext;
	TTaskRegisters *pNewRegs = m_pCurrent->GetRegs ();

	assert (pOldRegs != 0);
	assert (pNewRegs != 0);
	TaskSwitch (pOldRegs, pNewRegs);
}

void CScheduler::Sleep (unsigned nSeconds)
{
	// be sure the clock does not run over taken as signed int
	const unsigned nSleepMax = 1800;	// normally 2147 but to be sure
	while (nSeconds > nSleepMax)
	{
		usSleep (nSleepMax * 1000000);

		nSeconds -= nSleepMax;
	}

	usSleep (nSeconds * 1000000);
}

void CScheduler::MsSleep (unsigned nMilliSeconds)
{
	if (nMilliSeconds > 0)
	{
		usSleep (nMilliSeconds * 1000);
	}
}

void CScheduler::usSleep (unsigned nMicroSeconds)
{
	if (nMicroSeconds > 0)
	{
		unsigned nTicks = nMicroSeconds * (CLOCKHZ / 1000000);

		unsigned nStartTicks = CTimer::Get ()->GetClockTicks ();

		assert (m_pCurrent != 0);
		assert (m_pCurrent->GetState () == TaskStateReady);
		m_pCurrent->SetWakeTicks (nStartTicks + nTicks);
		m_pCurrent->SetState (TaskStateSleeping);

		Yield ();
	}
}

void CScheduler::AddTask (CTask *pTask)
{
	assert (pTask != 0);

	unsigned i;
	for (i = 0; i < m_nTasks; i++)
	{
		if (m_pTask[i] == 0)
		{
			m_pTask[i] = pTask;

			return;
		}
	}

	if (m_nTasks >= MAX_TASKS)
	{
		CLogger::Get ()->Write (FromScheduler, LogPanic, "System limit of tasks exceeded");
	}

	m_pTask[m_nTasks++] = pTask;
}

void CScheduler::RemoveTask (CTask *pTask)
{
	for (unsigned i = 0; i < m_nTasks; i++)
	{
		if (m_pTask[i] == pTask)
		{
			m_pTask[i] = 0;

			if (i == m_nTasks-1)
			{
				m_nTasks--;
			}

			return;
		}
	}

	assert (0);
}

void CScheduler::BlockTask (CTask **ppTask)
{
	assert (ppTask != 0);
	*ppTask = m_pCurrent;

	assert (m_pCurrent != 0);
	assert (m_pCurrent->GetState () == TaskStateReady);
	m_pCurrent->SetState (TaskStateBlocked);

	Yield ();
}

void CScheduler::WakeTask (CTask **ppTask)
{
	assert (ppTask != 0);
	CTask *pTask = *ppTask;

	*ppTask = 0;

#ifdef NDEBUG
	if (   pTask == 0
	    || pTask->GetState () != TaskStateBlocked)
	{
		CLogger::Get ()->Write (FromScheduler, LogPanic, "Tried to wake non-blocked task");
	}
#else
	assert (pTask != 0);
	assert (pTask->GetState () == TaskStateBlocked);
#endif

	pTask->SetState (TaskStateReady);
}

unsigned CScheduler::GetNextTask (void)
{
	unsigned nTask = m_nCurrent < MAX_TASKS ? m_nCurrent : 0;

	unsigned nTicks = CTimer::Get ()->GetClockTicks ();

	for (unsigned i = 1; i <= m_nTasks; i++)
	{
		if (++nTask >= m_nTasks)
		{
			nTask = 0;
		}

		CTask *pTask = m_pTask[nTask];
		if (pTask == 0)
		{
			continue;
		}

		switch (pTask->GetState ())
		{
		case TaskStateReady:
			return nTask;

		case TaskStateBlocked:
			continue;

		case TaskStateSleeping:
			if ((int) (pTask->GetWakeTicks () - nTicks) > 0)
			{
				continue;
			}
			pTask->SetState (TaskStateReady);
			return nTask;

		case TaskStateTerminated:
			RemoveTask (pTask);
			delete pTask;
			return MAX_TASKS;

		default:
			assert (0);
			break;
		}
	}

	return MAX_TASKS;
}

CScheduler *CScheduler::Get (void)
{
	assert (s_pThis != 0);
	return s_pThis;
}
