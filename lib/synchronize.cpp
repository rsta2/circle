//
// synchronize.cpp
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
#include <circle/synchronize.h>
#include <circle/sysconfig.h>
#include <circle/types.h>
#include <assert.h>

#define MAX_CRITICAL_LEVEL	20		// maximum nested level of EnterCritical()

unsigned CurrentExecutionLevel (void)
{
	u32 nCPSR;
	asm volatile ("mrs %0, cpsr" : "=r" (nCPSR));

	if (nCPSR & 0x40)
	{
		return FIQ_LEVEL;
	}

	if (nCPSR & 0x80)
	{
		return IRQ_LEVEL;
	}

	return TASK_LEVEL;
}

#ifdef ARM_ALLOW_MULTI_CORE

static volatile unsigned s_nCriticalLevel[CORES] = {0};
static volatile u32 s_nCPSR[CORES][MAX_CRITICAL_LEVEL];

void EnterCritical (unsigned nTargetLevel)
{
	assert (nTargetLevel == IRQ_LEVEL || nTargetLevel == FIQ_LEVEL);

	u32 nMPIDR;
	asm volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r" (nMPIDR));
	unsigned nCore = nMPIDR & (CORES-1);

	u32 nCPSR;
	asm volatile ("mrs %0, cpsr" : "=r" (nCPSR));

	// if we are already on FIQ_LEVEL, we must not go back to IRQ_LEVEL here
	assert (nTargetLevel == FIQ_LEVEL || !(nCPSR & 0x40));

	asm volatile ("cpsid if");	// disable both IRQ and FIQ

	assert (s_nCriticalLevel[nCore] < MAX_CRITICAL_LEVEL);
	s_nCPSR[nCore][s_nCriticalLevel[nCore]++] = nCPSR;

	if (nTargetLevel == IRQ_LEVEL)
	{
		EnableFIQs ();
	}

	DataMemBarrier ();
}

void LeaveCritical (void)
{
	u32 nMPIDR;
	asm volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r" (nMPIDR));
	unsigned nCore = nMPIDR & (CORES-1);

	DataMemBarrier ();

	DisableFIQs ();

	assert (s_nCriticalLevel[nCore] > 0);
	u32 nCPSR = s_nCPSR[nCore][--s_nCriticalLevel[nCore]];

	asm volatile ("msr cpsr_c, %0" :: "r" (nCPSR));
}

#else

static volatile unsigned s_nCriticalLevel = 0;
static volatile u32 s_nCPSR[MAX_CRITICAL_LEVEL];

void EnterCritical (unsigned nTargetLevel)
{
	assert (nTargetLevel == IRQ_LEVEL || nTargetLevel == FIQ_LEVEL);

	u32 nCPSR;
	asm volatile ("mrs %0, cpsr" : "=r" (nCPSR));

	// if we are already on FIQ_LEVEL, we must not go back to IRQ_LEVEL here
	assert (nTargetLevel == FIQ_LEVEL || !(nCPSR & 0x40));

	asm volatile ("cpsid if");	// disable both IRQ and FIQ

	assert (s_nCriticalLevel < MAX_CRITICAL_LEVEL);
	s_nCPSR[s_nCriticalLevel++] = nCPSR;

	if (nTargetLevel == IRQ_LEVEL)
	{
		EnableFIQs ();
	}

	DataMemBarrier ();
}

void LeaveCritical (void)
{
	DataMemBarrier ();

	DisableFIQs ();

	assert (s_nCriticalLevel > 0);
	u32 nCPSR = s_nCPSR[--s_nCriticalLevel];

	asm volatile ("msr cpsr_c, %0" :: "r" (nCPSR));
}

#endif

#if RASPPI == 1

//
// Cache maintenance operations for ARMv6
//
// NOTE: The following functions should hold all variables in CPU registers. Currently this will be
//	 ensured using maximum optimation (see circle/synchronize.h).
//

void CleanAndInvalidateDataCacheRange (u32 nAddress, u32 nLength)
{
	while (1)
	{
		asm volatile ("mcr p15, 0, %0, c7, c14,  1" : : "r" (nAddress) : "memory");

		if (nLength <= DATA_CACHE_LINE_LENGTH_MIN)
		{
			break;
		}

		nAddress += DATA_CACHE_LINE_LENGTH_MIN;
		nLength  -= DATA_CACHE_LINE_LENGTH_MIN;
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
