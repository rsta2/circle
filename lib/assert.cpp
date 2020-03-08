//
// assert.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#include <assert.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/interrupt.h>
#include <circle/synchronize.h>
#include <circle/sysconfig.h>
#include <circle/debug.h>
#include <circle/types.h>

#ifndef NDEBUG

void assertion_failed (const char *pExpr, const char *pFile, unsigned nLine)
{
	uintptr ulStackPtr;
	asm volatile ("mov %0,sp" : "=r" (ulStackPtr));

	// if an assertion fails on FIQ_LEVEL, the system would otherwise hang in the next spin lock
	CInterruptSystem::DisableFIQ ();
	EnableFIQs ();

	CString Source;
	Source.Format ("%s(%u)", pFile, nLine);

#ifndef USE_RPI_STUB_AT
	debug_stacktrace ((const uintptr *) ulStackPtr, Source);
	
	CLogger::Get ()->Write (Source, LogPanic, "assertion failed: %s", pExpr);
#else
	CLogger::Get ()->Write (Source, LogError, "assertion failed: %s", pExpr);

	Breakpoint (0);
#endif

	for (;;);
}

#endif
