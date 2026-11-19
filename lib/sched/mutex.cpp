//
// mutex.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2026  R. Stange <rsta2@gmx.net>
// 
// This class was developed by:
//	Brad Robinson <contact@toptensoftware.com>
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
#include <circle/sched/mutex.h>
#include <circle/sched/scheduler.h>
#include <circle/sched/task.h>
#include <circle/synchronize.h>
#include <circle/sysconfig.h>
#include <assert.h>

CMutex::CMutex (void)
:   m_pOwningTask (0),
    m_iReentrancyCount (0)
{
}

CMutex::~CMutex (void)
{
    assert(m_pOwningTask == 0);
}

// The calls of the scheduler and of the event pass the return address of the
// caller of CMutex, which a system panic on a secondary core names.

void CMutex::Acquire (void)
{
    const void *pCaller = __builtin_return_address(0);
    CTask* pTask = CScheduler::Get()->GetCurrentTask(pCaller);

    while (true)
    {
        if (m_pOwningTask == nullptr)
        {
            m_pOwningTask = pTask;
            m_iReentrancyCount = 1;
            return;
        }
        else if (m_pOwningTask == pTask)
        {
            m_iReentrancyCount++;
            return;
        }
        m_event.Wait(pCaller);
    }
}

boolean CMutex::TryAcquire (void)
{
    CTask* pTask = CScheduler::Get()->GetCurrentTask(__builtin_return_address(0));

    if (m_pOwningTask == nullptr)
    {
        m_pOwningTask = pTask;
        m_iReentrancyCount = 1;
        return true;
    }
    else if (m_pOwningTask == pTask)
    {
        m_iReentrancyCount++;
        return true;
    }
    return false;
}

void CMutex::Release (void)
{
    // asked in every build, a secondary core owns no mutex
    CTask* pTask = CScheduler::Get()->GetCurrentTask(__builtin_return_address(0));
    assert(m_pOwningTask == pTask);
    (void) pTask;   // to suppress the warning with NDEBUG

    m_iReentrancyCount--;
    if (m_iReentrancyCount == 0)
    {
        m_pOwningTask = 0;
        m_event.Pulse();
        CScheduler::Get()->Yield();
    }
}
