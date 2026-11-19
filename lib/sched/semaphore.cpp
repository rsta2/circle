//
// semaphore.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2021-2026  R. Stange <rsta2@gmx.net>
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
#include <circle/sched/semaphore.h>
#include <circle/atomic.h>
#include <assert.h>

CSemaphore::CSemaphore (unsigned nInitialCount)
:	m_nCount (nInitialCount),
	m_Event (m_nCount > 0)
{
	assert (m_nCount >= 0);
}

CSemaphore::~CSemaphore (void)
{
	assert (m_nCount >= 0);
}

unsigned CSemaphore::GetState (void) const
{
	return AtomicGet (&m_nCount);
}

// The waits pass the return address of the caller of CSemaphore, which the
// scheduler names for a wait on a secondary core.

void CSemaphore::Down (void)
{
	const void *pCaller = __builtin_return_address (0);

	while (AtomicGet (&m_nCount) == 0)
	{
		m_Event.Wait (pCaller);
	}

	if (AtomicDecrement (&m_nCount) == 0)
	{
		assert (m_Event.GetState ());
		m_Event.Clear ();
	}
}

boolean CSemaphore::DownWithTimeout (unsigned nMicroSeconds)
{
	const void *pCaller = __builtin_return_address (0);

	while (AtomicGet (&m_nCount) == 0)
	{
		if (   m_Event.WaitWithTimeout (nMicroSeconds, pCaller)
		    && !m_Event.GetState ())
		{
			return TRUE;
		}
	}

	if (AtomicDecrement (&m_nCount) == 0)
	{
		assert (m_Event.GetState ());
		m_Event.Clear ();
	}

	return FALSE;
}

void CSemaphore::Up (void)
{
	if (AtomicIncrement (&m_nCount) == 1)
	{
		assert (!m_Event.GetState ());
		m_Event.Set ();
	}
#if NDEBUG
	else
	{
		assert (m_Event.GetState ());
	}
#endif

}

boolean CSemaphore::TryDown (void)
{
	if (AtomicGet (&m_nCount) == 0)
	{
		return FALSE;
	}

	if (AtomicDecrement (&m_nCount) == 0)
	{
		assert (m_Event.GetState ());
		m_Event.Clear ();
	}

	return TRUE;
}
