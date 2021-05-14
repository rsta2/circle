//
// synchronizationevent.h
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
#ifndef _circle_sched_synchronizationevent_h
#define _circle_sched_synchronizationevent_h

#include <circle/types.h>

class CTask;

class CSynchronizationEvent /// Provides a method to synchronize the execution of a task with an event
{
public:
	/// \param bState Initial state of the event (default cleared)
	CSynchronizationEvent (boolean bState = FALSE);

	~CSynchronizationEvent (void);

	/// \return Event set?
	boolean GetState (void);

	/// \brief Clear the event
	void Clear (void);
	/// \brief Set the event; wakes all task(s) currently waiting for the event
	/// \note Can be called from interrupt context.
	void Set (void);

	/// \brief Block the calling task, if the event is cleared
	/// \note The task will wake up, when the event is set later.
	/// \note Multiple tasks can wait for the event to be set.
	void Wait (void);

	/// \brief Wait for this event to be set, or a time period to elapse
	/// \param nMicroSeconds Timeout in micro seconds
	/// \return TRUE if timed out
	/// \note To determine what caused the method to return use GetState() to see,\n
	///	  if the event has been set.
	/// \note It is possible to have timed out and for the event to be set.
	boolean WaitWithTimeout (unsigned nMicroSeconds);

private:
	void Pulse (void);	// wakes all waiting tasks without actually setting the event
	friend class CMutex;

private:
	volatile boolean m_bState;
	CTask	*m_pWaitListHead;	// Linked list of waiting tasks
};

#endif
