//
// task.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_sched_task_h
#define _circle_sched_task_h

#include <circle/sched/taskswitch.h>
#include <circle/sysconfig.h>
#include <circle/types.h>

enum TTaskState
{
	TaskStateReady,
	TaskStateBlocked,
	TaskStateTerminated,
	TaskStateUnknown
};

class CScheduler;

class CTask
{
public:
	CTask (unsigned nStackSize = TASK_STACK_SIZE);		// nStackSize = 0 for main task
	virtual ~CTask (void);

	virtual void Run (void);

private:
	TTaskState GetState (void) const;
	void SetState (TTaskState State);

	TTaskRegisters *GetRegs (void);

	friend class CScheduler;

private:
	void InitializeRegs (void);

	static void TaskEntry (void *pParam);

private:
	volatile TTaskState m_State;
	TTaskRegisters	    m_Regs;
	unsigned	    m_nStackSize;
	u8		   *m_pStack;
};

#endif
