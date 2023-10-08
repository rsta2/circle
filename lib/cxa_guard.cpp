//
// cxa_guard.cpp
//
// Guard functions, used to initialize static objects inside a function
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

//
// The semantic of the functions __cxa_guard_*() herein is based
// on the file cxa_guard.cxx of the LLVM Compiler Infrastructure,
// with this license:
//
// ==============================================================================
// LLVM Release License
// ==============================================================================
// University of Illinois/NCSA
// Open Source License
//
// Copyright (c) 2003-2010 University of Illinois at Urbana-Champaign.
// All rights reserved.
//
// Developed by:
//
//     LLVM Team
//
//     University of Illinois at Urbana-Champaign
//
//     http://llvm.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal with
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimers.
//
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimers in the
//       documentation and/or other materials provided with the distribution.
//
//     * Neither the names of the LLVM Team, University of Illinois at
//       Urbana-Champaign, nor the names of its contributors may be used to
//       endorse or promote products derived from this Software without specific
//       prior written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
// SOFTWARE.
//

#include <circle/multicore.h>
#include <circle/atomic.h>
#include <circle/types.h>

// byte index into the guard object
#define INDEX_HAS_RUN		0
#define INDEX_IN_USE		1

#ifdef ARM_ALLOW_MULTI_CORE

// a global lock, which can be acquired recursively

#define LOCK_FREE	CORES
static volatile int s_nGuardCore = LOCK_FREE;
static volatile int s_nGuardAcquireCount = 0;

static void GuardAcquire (void)
{
	int nThisCore = (int) CMultiCoreSupport::ThisCore ();

	int nGuardCore;
	while (      (nGuardCore = AtomicCompareExchange (&s_nGuardCore, LOCK_FREE, nThisCore))
		  != LOCK_FREE
	       && nGuardCore != nThisCore)
	{
		// just wait
	}

	s_nGuardAcquireCount++;
}

static void GuardRelease (void)
{
	assert (s_nGuardAcquireCount > 0);
	if (--s_nGuardAcquireCount == 0)
	{
		AtomicSet (&s_nGuardCore, LOCK_FREE);
	}
}

#endif

extern "C" int __cxa_guard_acquire (volatile u8 *pGuardObject)
{
	if (pGuardObject[INDEX_HAS_RUN] != 0)
	{
		return 0;	// do not run constructor
	}

#ifdef ARM_ALLOW_MULTI_CORE
	GuardAcquire ();

	if (pGuardObject[INDEX_HAS_RUN] != 0)	// did another core locked the guard?
	{
		GuardRelease ();

		return 0;
	}
#endif

	assert (pGuardObject[INDEX_IN_USE] == 0);
	pGuardObject[INDEX_IN_USE] = 1;

	return 1;		// run constructor
}

extern "C" void __cxa_guard_release (volatile u8 *pGuardObject)
{
	pGuardObject[INDEX_HAS_RUN] = 1;

#ifdef ARM_ALLOW_MULTI_CORE
	GuardRelease ();
#endif
}

#if 0	// not needed, because we do not support exceptions with STDLIB_SUPPORT < 3 here

extern "C" void __cxa_guard_abort (volatile u8 *pGuardObject)
{
#ifdef ARM_ALLOW_MULTI_CORE
	GuardRelease ();
#endif

	pGuardObject[INDEX_IN_USE] = 0;
}

#endif
