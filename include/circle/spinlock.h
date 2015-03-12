//
// spinlock.h
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
#ifndef _circle_spinlock_h
#define _circle_spinlock_h

#include <circle/sysconfig.h>
#include <circle/synchronize.h>
#include <circle/types.h>

#ifdef ARM_ALLOW_MULTI_CORE

class CSpinLock
{
public:
	// call constructor with bDisableIRQ = FALSE if spin lock is not used from IRQ context
	CSpinLock (boolean bDisableIRQ = TRUE);
	~CSpinLock (void);

	void Acquire (void);
	void Release (void);

	static void Enable (void);

private:
	boolean m_bDisableIRQ;
	u32 m_nCPSR[CORES];

	boolean m_bLocked;

	static boolean s_bEnabled;
};

#else

class CSpinLock
{
public:
	CSpinLock (boolean bDisableIRQ = TRUE)
	:	m_bDisableIRQ (bDisableIRQ)
	{
	}

	void Acquire (void)
	{
		if (m_bDisableIRQ)
		{
			EnterCritical ();
		}
	}

	void Release (void)
	{
		if (m_bDisableIRQ)
		{
			LeaveCritical ();
		}
	}

private:
	boolean m_bDisableIRQ;
};

#endif

#endif
