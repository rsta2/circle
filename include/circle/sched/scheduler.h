//
/// \file scheduler.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2026  R. Stange <rsta2@o2online.de>
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
#include <circle/macros.h>
#include <circle/types.h>

enum TTaskFlags		///< for EnumerateTasks()
{
	TaskFlagNone		= 0,
	TaskFlagRunning		= BIT (0),
	TaskFlagSuspended	= BIT (1)
};

typedef void TSchedulerTaskHandler (CTask *pTask);

#define MAX_SECONDARY_CORE_CALLS	0x7FFFFFFF	///< CScheduler::GetSecondaryCoreCalls() stops here
#ifndef MAX_SECONDARY_CORE_CALLERS
#define MAX_SECONDARY_CORE_CALLERS	16		///< return addresses of these calls that are logged
#endif

/// \note This scheduler uses the round-robin policy, without priorities.

class CScheduler /// Cooperative non-preemptive scheduler, which controls which task runs at a time
{
public:
	CScheduler (void);
	~CScheduler (void);

	/// \brief Switch to the next task
	/// \note A task should call this from time to time, if it does longer calculations.
	/// \note Does not switch the task on a secondary core, see GetSecondaryCoreCalls().
	void Yield (void);

	/// \param nSeconds Number of seconds, the current task will be sleep
	/// \note On a secondary core the sleep methods delay the calling core instead,\n
	///	  see GetSecondaryCoreCalls().
	void Sleep (unsigned nSeconds);
	/// \param nMilliSeconds Number of milliseconds, the current task will be sleep
	void MsSleep (unsigned nMilliSeconds);
	/// \param nMicroSeconds Number of microseconds, the current task will be sleep
	void usSleep (unsigned nMicroSeconds);

	/// \return Number of calls of Yield(), Sleep(), MsSleep() and usSleep()\n
	///	    on a secondary core, and of waits there for a CSynchronizationEvent\n
	///	    that is already set
	/// \note The tasks run on core 0 only, so these calls do not switch the task.\n
	///	  The first call from each return address is logged with its core,\n
	///	  for up to MAX_SECONDARY_CORE_CALLERS different return addresses.
	/// \note The count stops at MAX_SECONDARY_CORE_CALLS and is always 0\n
	///	  without ARM_ALLOW_MULTI_CORE.
	unsigned GetSecondaryCoreCalls (void) const;

	/// \return Pointer to the CTask object of the currently running task
	/// \note A secondary core has no current task, a call there is a system panic.
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

	/// \brief Enumerate all tasks
	/// \param pCallback A callback to be invoked for each task
	/// \param pParam A user define pointer that will back passed to the callback
	/// \return FALSE if the enumeration was cancelled by the callback returning FALSE
	boolean EnumerateTasks (
		boolean (*pCallback) (CTask *pTask, const char *pName,
				      TTaskState State, TTaskFlags Flags,
				      void *pParam),
		void *pParam
	);

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
	// pCaller is the return address of the constructor of the task. A task
	// cannot be created on a secondary core, a call there is a system panic.
	void AddTask (CTask *pTask, const void *pCaller);
	friend class CTask;

	// pCaller is the return address of the wait. A secondary core has no task
	// that could be blocked, so a call there is a system panic.
	// pState is an optional pointer to a synchronization event variable, which is
	// atomically checked with the blocking operation. When is specified and is set,
	// the method returns silently, on a secondary core after counting the call.
	boolean BlockTask (CTask **ppWaitListHead, unsigned nMicroSeconds,
			    const void *pCaller, const volatile boolean *pState = 0);
	void WakeTasks (CTask **ppWaitListHead); // can be called from interrupt context
	friend class CSynchronizationEvent;

	// GetCurrentTask() for CMutex, pCaller is the return address the system panic
	// on a secondary core names, which is the one of the caller of CMutex
	CTask *GetCurrentTask (const void *pCaller);
	friend class CMutex;

	void RemoveTask (CTask *pTask);
	unsigned GetNextTask (void); // returns index into m_pTask or MAX_TASKS if no task was found

	// bSecondaryCore is the result of CountSecondaryCoreCall() for the sleep method
	void usSleepOnCore (unsigned nMicroSeconds, boolean bSecondaryCore);

	// The checks of the calling core, which is always core 0 without
	// ARM_ALLOW_MULTI_CORE. pCaller is the return address of the checked call,
	// and pOperation names the call in the log.
	// returns TRUE on a secondary core, where it counts the call and logs it,
	// if it is the first one from pCaller
	boolean CountSecondaryCoreCall (const char *pOperation, const void *pCaller);
	// is a system panic on a secondary core
	void PanicOnSecondaryCore (const char *pOperation, const void *pCaller);

	// returns TRUE once for each pCaller, for up to MAX_SECONDARY_CORE_CALLERS of them
	boolean IsNewSecondaryCoreCaller (const void *pCaller);

private:
	CTask *m_pTask[MAX_TASKS];
	unsigned m_nTasks;

	CTask *m_pCurrent;
	unsigned m_nCurrent;	// index into m_pTask

	TSchedulerTaskHandler *m_pTaskSwitchHandler;
	TSchedulerTaskHandler *m_pTaskTerminationHandler;

	int m_iSuspendNewTasks;

	volatile int m_nSecondaryCoreCalls;	// see GetSecondaryCoreCalls()

	// the return addresses IsNewSecondaryCoreCaller() has seen, guarded by its lock
	const void *m_pSecondaryCoreCaller[MAX_SECONDARY_CORE_CALLERS];
	unsigned m_nSecondaryCoreCallers;
	CSpinLock m_SecondaryCoreCallerLock;

	CSpinLock m_SpinLock;

	static CScheduler *s_pThis;
};

#endif
