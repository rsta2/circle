//
// mutex.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2021  R. Stange <rsta2@o2online.de>
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

void CMutex::Acquire (void)
{
    CTask* pTask = CScheduler::Get()->GetCurrentTask();

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
        m_event.Wait();
    }
}

void CMutex::Release (void)
{
    assert(m_pOwningTask == CScheduler::Get()->GetCurrentTask());
    m_iReentrancyCount--;
    if (m_iReentrancyCount == 0)
    {
        m_pOwningTask = 0;
        m_event.Pulse();
        CScheduler::Get()->Yield();
    }
}
