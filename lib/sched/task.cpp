//
// task.cpp
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
#include <circle/sched/task.h>
#include <circle/sched/scheduler.h>
#include <circle/util.h>
#include <assert.h>

CTask::CTask (unsigned nStackSize)
:	m_State (TaskStateReady),
	m_nStackSize (nStackSize),
	m_pStack (0)
{
	if (m_nStackSize != 0)
	{
		assert (m_nStackSize >= 1024);
		assert ((m_nStackSize & 3) == 0);
		m_pStack = new u8[m_nStackSize];
		assert (m_pStack != 0);

		InitializeRegs ();
	}

	CScheduler::Get ()->AddTask (this);
}

CTask::~CTask (void)
{
	assert (m_State == TaskStateTerminated);
	m_State = TaskStateUnknown;

	delete [] m_pStack;
	m_pStack = 0;
}

void CTask::Run (void)		// dummy method which is never called
{
	assert (0);
}

void CTask::InitializeRegs (void)
{
	memset (&m_Regs, 0, sizeof m_Regs);

	m_Regs.r0 = (u32) this;		// pParam for TaskEntry()

	assert (m_pStack != 0);
	m_Regs.sp = (u32) m_pStack + m_nStackSize;

	m_Regs.lr = (u32) &TaskEntry;
}

void CTask::TaskEntry (void *pParam)
{
	CTask *pThis = (CTask *) pParam;
	assert (pThis != 0);

	pThis->Run ();

	pThis->m_State = TaskStateTerminated;
	CScheduler::Get ()->Yield ();

	assert (0);
}
