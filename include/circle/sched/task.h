//
// task.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2020  R. Stange <rsta2@o2online.de>
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
#include <circle/sched/synchronizationevent.h>
#include <circle/sysconfig.h>
#include <circle/types.h>

enum TTaskState
{
	TaskStateReady,
	TaskStateBlocked,
	TaskStateSleeping,
	TaskStateTerminated,
	TaskStateNew,
	TaskStateBlockedWithTimeout,
	TaskStateUnknown
};

class CScheduler;

class CTask
{
public:
	CTask (unsigned nStackSize = TASK_STACK_SIZE, boolean createSuspended = false);		// nStackSize = 0 for main task
	virtual ~CTask (void);

	// Starts a task that was created in suspended mode
	void Start (void);

	virtual void Run (void);

	void Terminate (void);			// callable from this task only
	void WaitForTermination (void);		// callable from other task only

#define TASK_USER_DATA_KTHREAD		0	// Linux driver emulation
#define TASK_USER_DATA_ERROR_STACK	1	// Plan 9 driver emulation
#define TASK_USER_DATA_SLOTS		2
	void SetUserData (void *pData, unsigned nSlot);
	void *GetUserData (unsigned nSlot);

private:
	TTaskState GetState (void) const	{ return m_State; }
	void SetState (TTaskState State)	{ m_State = State; }

	unsigned GetWakeTicks (void) const	{ return m_nWakeTicks; }
	void SetWakeTicks (unsigned nTicks)	{ m_nWakeTicks = nTicks; }

	TTaskRegisters *GetRegs (void)		{ return &m_Regs; }

	friend class CScheduler;

private:
	void InitializeRegs (void);

	static void TaskEntry (void *pParam);

private:
	volatile TTaskState m_State;
	unsigned	    m_nWakeTicks;
	TTaskRegisters	    m_Regs;
	unsigned	    m_nStackSize;
	u8		   *m_pStack;
	void		   *m_pUserData[TASK_USER_DATA_SLOTS];
	CSynchronizationEvent m_Event;
	CTask*			m_pWaitListNext;	// next in list of tasks waiting on an event
};

#endif
