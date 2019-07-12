//
// multicore.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_multicore_h
#define _circle_multicore_h

#include <circle/sysconfig.h>

#ifdef ARM_ALLOW_MULTI_CORE

#include <circle/memory.h>
#include <circle/types.h>

// inter-processor interrupt (IPI)
#define IPI_HALT_CORE		0		// halt target core
#define IPI_USER		10		// first user defineable IPI
#if RASPPI <= 3
#define IPI_MAX			31
#else
#define IPI_MAX			15
#endif

class CMultiCoreSupport
{
public:
	CMultiCoreSupport (CMemorySystem *pMemorySystem);
	virtual ~CMultiCoreSupport (void);

	boolean Initialize (void);

	virtual void Run (unsigned nCore) = 0;

	virtual void IPIHandler (unsigned nCore, unsigned nIPI);	// handles IPI on this core

public:
	static void SendIPI (unsigned nCore, unsigned nIPI);		// send IPI to core
	static void HaltAll (void);					// halt all cores

#if RASPPI <= 3
	static boolean LocalInterruptHandler (void);	// returns TRUE if local interrupt was handled
#else
	static void LocalInterruptHandler (unsigned nFromCore, unsigned nIPI);
#endif

	static unsigned ThisCore (void)			// returns number of current core (0..CORES-1)
	{
#if AARCH == 32
		u32 nMPIDR;
		asm volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r" (nMPIDR));
#else
		u64 nMPIDR;
		asm volatile ("mrs %0, mpidr_el1" : "=r" (nMPIDR));
#endif

		return nMPIDR & (CORES-1);
	}

	static void EntrySecondary (void);

private:
	CMemorySystem *m_pMemorySystem;

	static CMultiCoreSupport *s_pThis;
};

#endif

#endif
