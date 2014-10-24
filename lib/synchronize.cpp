//
// synchronize.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#include <circle/synchronize.h>
#include <circle/types.h>
#include <assert.h>

static volatile unsigned s_nCriticalLevel = 0;
static volatile boolean s_bWereEnabled;

void EnterCritical (void)
{
	u32 nFlags;
	asm volatile ("mrs %0, cpsr" : "=r" (nFlags));

	DisableInterrupts ();

	if (s_nCriticalLevel++ == 0)
	{
		s_bWereEnabled = nFlags & 0x80 ? FALSE : TRUE;
	}

	DataMemBarrier ();
}

void LeaveCritical (void)
{
	DataMemBarrier ();

	assert (s_nCriticalLevel > 0);
	if (--s_nCriticalLevel == 0)
	{
		if (s_bWereEnabled)
		{
			EnableInterrupts ();
		}
	}
}
