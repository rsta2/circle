//
// genericlock.h
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
#ifndef _circle_genericlock_h
#define _circle_genericlock_h

#include <circle/sysconfig.h>

#ifdef NO_BUSY_WAIT
	#include <circle/sched/mutex.h>
#else
	#include <circle/spinlock.h>
#endif

class CGenericLock	/// Locks a resource with or without scheduler
{
public:
	CGenericLock (void)
#ifndef NO_BUSY_WAIT
	:	m_SpinLock (TASK_LEVEL)
#endif
	{
	}

	void Acquire (void)
	{
#ifdef NO_BUSY_WAIT
		m_Mutex.Acquire ();
#else
		m_SpinLock.Acquire ();
#endif
	}

	void Release (void)
	{
#ifdef NO_BUSY_WAIT
		m_Mutex.Release ();
#else
		m_SpinLock.Release ();
#endif
	}

private:
#ifdef NO_BUSY_WAIT
	CMutex m_Mutex;
#else
	CSpinLock m_SpinLock;
#endif
};

#endif
