//
/// \file scheduler.h
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
#ifndef _circle_sched_scheduler_h
#define _circle_sched_scheduler_h

#include <circle/sched/task.h>
#include <circle/spinlock.h>
#include <circle/device.h>
#include <circle/sysconfig.h>
#include <circle/types.h>

typedef void TSchedulerTaskHandler (CTask *pTask);

/// \note This scheduler uses the round-robin policy, without priorities.

class CScheduler /// Cooperative non-preemtive scheduler, which controls which task runs at a time
{
public:
	CScheduler (void);
	~CScheduler (void);

	/// \brief Switch to the next task
	/// \note A task should call this from time to time, if it does longer calculations.
	void Yield (void);

	/// \param nSeconds Number of seconds, the current task will be sleep
	void Sleep (unsigned nSeconds);
	/// \param nMilliSeconds Number of milliseconds, the current task will be sleep
	void MsSleep (unsigned nMilliSeconds);
	/// \param nMicroSeconds Number of microseconds, the current task will be sleep
	void usSleep (unsigned nMicroSeconds);

	/// \return Pointer to the CTask object of the currently running task
	CTask *GetCurrentTask (void);

	/// \param pTaskName Task name to look for
	/// \return Pointer to the CTask object of the task with the given name (0 if not found)
	CTask *GetTask (const char *pTaskName);

	/// \param pTask Any pointer
	/// \return Is is this pointer referencing a CTask object of a currently known task?
	boolean IsValidTask (CTask *pTask);

	/// \param pHandler Callback, which is called on each task switch
	/// \note The handler is called with a pointer to the CTask object of the task,\n
	///	  which gets control now.
	void RegisterTaskSwitchHandler (TSchedulerTaskHandler *pHandler);
	/// \param pHandler Callback, which is called, when a task terminates
	/// \note The handler is called with a pointer to the CTask object of the task,\n
	///	  which terminates.
	void RegisterTaskTerminationHandler (TSchedulerTaskHandler *pHandler);

	/// \brief Causes all new tasks to be created in a suspended state
	/// \note Nested calls to SuspendNewTasks() and ResumeNewTasks() are allowed.
	void SuspendNewTasks (void);
	/// \brief Stops causing new tasks to be created in a suspended state\n
	///	   and starts any tasks that were created suspended.
	void ResumeNewTasks (void);

	/// \brief Generate task listing
	/// \param pTarget Device to be used for output
	void ListTasks (CDevice *pTarget);

	/// \return Pointer to the only scheduler object in the system
	static CScheduler *Get (void);

	/// \return Is the scheduler available in the system?
	/// \note The scheduler is optional in Circle.
	static boolean IsActive (void)
	{
		return s_pThis != 0 ? TRUE : FALSE;
	}

private:
	void AddTask (CTask *pTask);
	friend class CTask;

	boolean BlockTask (CTask **ppWaitListHead, unsigned nMicroSeconds);
	void WakeTasks (CTask **ppWaitListHead); // can be called from interrupt context
	friend class CSynchronizationEvent;

	void RemoveTask (CTask *pTask);
	unsigned GetNextTask (void); // returns index into m_pTask or MAX_TASKS if no task was found

private:
	CTask *m_pTask[MAX_TASKS];
	unsigned m_nTasks;

	CTask *m_pCurrent;
	unsigned m_nCurrent;	// index into m_pTask

	TSchedulerTaskHandler *m_pTaskSwitchHandler;
	TSchedulerTaskHandler *m_pTaskTerminationHandler;

	int m_iSuspendNewTasks;

	CSpinLock m_SpinLock;

	static CScheduler *s_pThis;
};

#endif
