//
/// task.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/string.h>
#include <circle/types.h>

enum TTaskState
{
	TaskStateNew,
	TaskStateReady,
	TaskStateBlocked,
	TaskStateBlockedWithTimeout,
	TaskStateSleeping,
	TaskStateTerminated,
	TaskStateUnknown
};

class CScheduler;

class CTask	/// Overload this class, define the Run() method, and call new on it to start it.
{
public:
	/// \param nStackSize Stack size for this task (0 used internally for the main task)
	/// \param bCreateSuspended Set to TRUE, if the task is initially not ready to run
	CTask (unsigned nStackSize = TASK_STACK_SIZE, boolean bCreateSuspended = FALSE);

	virtual ~CTask (void);

	/// \brief Override this method to define the entry point for your class
	virtual void Run (void);

	/// \brief Starts a task that was created with bCreateSuspended = TRUE\n
	/// or restarts it after Suspend()
	void Start (void);
	/// \brief Suspend task from running until Resume()
	void Suspend (void);
	/// \brief Alternative method to (re-)start suspended task
	void Resume (void)			{ Start (); }
	/// \return Is task suspended from running?
	boolean IsSuspended (void) const	{ return m_bSuspended; }

	/// \brief Terminate the execution of this task
	/// \note Callable from this task only
	/// \note The task terminates on return from Run() too.
	void Terminate (void);
	/// \brief Wait for the termination of this task
	/// \note Callable from other task only
	void WaitForTermination (void);

	/// \brief Set a specific name for this task
	/// \param pName Name string for this task
	void SetName (const char *pName);
	/// \return Pointer to 0-terminated name string ("@this_address" if not explicitly set)
	const char *GetName (void) const;

#define TASK_USER_DATA_KTHREAD		0	// Linux driver emulation
#define TASK_USER_DATA_ERROR_STACK	1	// Plan 9 driver emulation
#define TASK_USER_DATA_USER		2	// Free for application usage
#define TASK_USER_DATA_SLOTS		3	// Number of available slots
	/// \brief Set a user pointer for this task
	/// \param pData Any user pointer
	/// \param nSlot The slot to be set
	/// \note The slot TASK_USER_DATA_USER is free for application use.
	void SetUserData (void *pData, unsigned nSlot);
	/// \brief Get user pointer from slot
	/// \param nSlot The slot to be read
	/// \return Any user pointer, previously set with SetUserData()
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
	boolean		    m_bSuspended;
	unsigned	    m_nWakeTicks;
	TTaskRegisters	    m_Regs;
	unsigned	    m_nStackSize;
	u8		   *m_pStack;
	CString		    m_Name;
	void		   *m_pUserData[TASK_USER_DATA_SLOTS];
	CSynchronizationEvent m_Event;
	CTask		   *m_pWaitListNext;	// next in list of tasks waiting on an event
};

#endif
