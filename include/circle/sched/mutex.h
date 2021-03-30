//
// mutex.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2021  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_sched_mutex_h
#define _circle_sched_mutex_h

#include <circle/types.h>
#include <circle/sched/synchronizationevent.h>

class CTask;

class CMutex	/// Provides a method to provide mutual exclusion (critical sections) across tasks
{
public:
	CMutex (void);
	~CMutex (void);

	/// \brief Acquire the mutex; task blocks, if another task already acquired the mutex
	/// \note This mutex can be acquired multiple times by the same task.
	void Acquire (void);

	/// \brief Release the mutex; wake another task, which was waiting for the mutex
	void Release (void);

private:
	CTask* m_pOwningTask;
	int m_iReentrancyCount;
	CSynchronizationEvent m_event;
};

#endif
