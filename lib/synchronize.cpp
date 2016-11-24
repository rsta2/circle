//
// synchronize.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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
#include <circle/sysconfig.h>
#include <circle/types.h>
#include <assert.h>

#ifdef ARM_ALLOW_MULTI_CORE

static volatile unsigned s_nCriticalLevel[CORES] = {0};
static volatile boolean s_bWereEnabled[CORES];

void EnterCritical (void)
{
	u32 nMPIDR;
	asm volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r" (nMPIDR));
	unsigned nCore = nMPIDR & (CORES-1);

	u32 nFlags;
	asm volatile ("mrs %0, cpsr" : "=r" (nFlags));

	DisableInterrupts ();

	if (s_nCriticalLevel[nCore]++ == 0)
	{
		s_bWereEnabled[nCore] = nFlags & 0x80 ? FALSE : TRUE;
	}

	DataMemBarrier ();
}

void LeaveCritical (void)
{
	u32 nMPIDR;
	asm volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r" (nMPIDR));
	unsigned nCore = nMPIDR & (CORES-1);

	DataMemBarrier ();

	assert (s_nCriticalLevel[nCore] > 0);
	if (--s_nCriticalLevel[nCore] == 0)
	{
		if (s_bWereEnabled[nCore])
		{
			EnableInterrupts ();
		}
	}
}

#else

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

#endif

#if RASPPI == 1

//
// Cache maintenance operations for ARMv6
//
// NOTE: The following functions should hold all variables in CPU registers. Currently this will be
//	 ensured using maximum optimation (see circle/synchronize.h).
//
//	 The following numbers can be determined (dynamically) using CTR.
//	 As long we use the ARM1176JZF-S implementation in the BCM2835 these static values will work:
//

#define DATA_CACHE_LINE_LENGTH		32

void CleanAndInvalidateDataCacheRange (u32 nAddress, u32 nLength)
{
	nLength += DATA_CACHE_LINE_LENGTH;

	while (1)
	{
		asm volatile ("mcr p15, 0, %0, c7, c14,  1" : : "r" (nAddress) : "memory");

		if (nLength < DATA_CACHE_LINE_LENGTH)
		{
			break;
		}

		nAddress += DATA_CACHE_LINE_LENGTH;
		nLength  -= DATA_CACHE_LINE_LENGTH;
	}

	DataSyncBarrier ();
}

#endif

// ARMv7 ARM A3.5.4
void SyncDataAndInstructionCache (void)
{
	CleanDataCache ();
	//DataSyncBarrier ();		// included in CleanDataCache()

	InvalidateInstructionCache ();
	FlushBranchTargetCache ();
	DataSyncBarrier ();

	InstructionSyncBarrier ();
}
