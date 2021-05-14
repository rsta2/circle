//
// semaphore.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2021  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_sched_semaphore_h
#define _circle_sched_semaphore_h

#include <circle/sched/synchronizationevent.h>
#include <circle/types.h>

class CSemaphore	/// Implements a semaphore synchronization class
{
public:
	/// \param nInitialCount Initial count of the semaphore
	CSemaphore (unsigned nInitialCount = 1);

	~CSemaphore (void);

	/// \return Current semaphore count
	unsigned GetState (void) const;

	/// \brief Decrement semaphore count; block task, if count is already 0
	void Down (void);

	/// \brief Increment semaphore count; wake another waiting task, if count was 0
	/// \note Can be called from interrupt context.
	void Up (void);

	/// \brief Try to decrement semaphore count
	/// \return Operation successful?
	boolean TryDown (void);

private:
	volatile int m_nCount;

	CSynchronizationEvent m_Event;
};

#endif
