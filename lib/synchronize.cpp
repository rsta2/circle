//
// synchronize.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2015  R. Stange <rsta2@o2online.de>
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

#if RASPPI != 1

//
// Cache maintenance operations for ARMv7-A
//
// See: ARMv7-A Architecture Reference Manual, Section B4.2.1
//
// NOTE: The following functions should hold all variables in CPU registers. Currently this will be
//	 ensured using the register keyword and maximum optimation (see circle/synchronize.h).
//
//	 The following numbers can be determined (dynamically) using CTR, CSSELR, CCSIDR and CLIDR.
//	 As long we use the Cortex-A7 implementation in the BCM2836 these static values will work:
//

#define SETWAY_LEVEL_SHIFT		1

#define L1_DATA_CACHE_SETS		128
#define L1_DATA_CACHE_WAYS		4
	#define L1_SETWAY_WAY_SHIFT		30	// 32-Log2(L1_DATA_CACHE_WAYS)
#define L1_DATA_CACHE_LINE_LENGTH	64
	#define L1_SETWAY_SET_SHIFT		6	// Log2(L1_DATA_CACHE_LINE_LENGTH)

#define L2_CACHE_SETS			1024
#define L2_CACHE_WAYS			8
	#define L2_SETWAY_WAY_SHIFT		29	// 32-Log2(L2_CACHE_WAYS)
#define L2_CACHE_LINE_LENGTH		64
	#define L2_SETWAY_SET_SHIFT		6	// Log2(L2_CACHE_LINE_LENGTH)

#define DATA_CACHE_LINE_LENGTH_MIN	64		// min(L1_DATA_CACHE_LINE_LENGTH, L2_CACHE_LINE_LENGTH)

void InvalidateDataCache (void)
{
	// invalidate L1 data cache
	for (register unsigned nSet = 0; nSet < L1_DATA_CACHE_SETS; nSet++)
	{
		for (register unsigned nWay = 0; nWay < L1_DATA_CACHE_WAYS; nWay++)
		{
			register u32 nSetWayLevel =   nWay << L1_SETWAY_WAY_SHIFT
						    | nSet << L1_SETWAY_SET_SHIFT
						    | 0 << SETWAY_LEVEL_SHIFT;

			asm volatile ("mcr p15, 0, %0, c7, c6,  2" : : "r" (nSetWayLevel) : "memory");	// DCISW
		}
	}

	// invalidate L2 unified cache
	for (register unsigned nSet = 0; nSet < L2_CACHE_SETS; nSet++)
	{
		for (register unsigned nWay = 0; nWay < L2_CACHE_WAYS; nWay++)
		{
			register u32 nSetWayLevel =   nWay << L2_SETWAY_WAY_SHIFT
						    | nSet << L2_SETWAY_SET_SHIFT
						    | 1 << SETWAY_LEVEL_SHIFT;

			asm volatile ("mcr p15, 0, %0, c7, c6,  2" : : "r" (nSetWayLevel) : "memory");	// DCISW
		}
	}
}

void InvalidateDataCacheL1Only (void)
{
	// invalidate L1 data cache
	for (register unsigned nSet = 0; nSet < L1_DATA_CACHE_SETS; nSet++)
	{
		for (register unsigned nWay = 0; nWay < L1_DATA_CACHE_WAYS; nWay++)
		{
			register u32 nSetWayLevel =   nWay << L1_SETWAY_WAY_SHIFT
						    | nSet << L1_SETWAY_SET_SHIFT
						    | 0 << SETWAY_LEVEL_SHIFT;

			asm volatile ("mcr p15, 0, %0, c7, c6,  2" : : "r" (nSetWayLevel) : "memory");	// DCISW
		}
	}
}

void CleanDataCache (void)
{
	// clean L1 data cache
	for (register unsigned nSet = 0; nSet < L1_DATA_CACHE_SETS; nSet++)
	{
		for (register unsigned nWay = 0; nWay < L1_DATA_CACHE_WAYS; nWay++)
		{
			register u32 nSetWayLevel =   nWay << L1_SETWAY_WAY_SHIFT
						    | nSet << L1_SETWAY_SET_SHIFT
						    | 0 << SETWAY_LEVEL_SHIFT;

			asm volatile ("mcr p15, 0, %0, c7, c10,  2" : : "r" (nSetWayLevel) : "memory");	// DCCSW
		}
	}

	// clean L2 unified cache
	for (register unsigned nSet = 0; nSet < L2_CACHE_SETS; nSet++)
	{
		for (register unsigned nWay = 0; nWay < L2_CACHE_WAYS; nWay++)
		{
			register u32 nSetWayLevel =   nWay << L2_SETWAY_WAY_SHIFT
						    | nSet << L2_SETWAY_SET_SHIFT
						    | 1 << SETWAY_LEVEL_SHIFT;

			asm volatile ("mcr p15, 0, %0, c7, c10,  2" : : "r" (nSetWayLevel) : "memory");	// DCCSW
		}
	}
}

void InvalidateDataCacheRange (u32 nAddress, u32 nLength)
{
	nLength += DATA_CACHE_LINE_LENGTH_MIN;

	while (1)
	{
		asm volatile ("mcr p15, 0, %0, c7, c6,  1" : : "r" (nAddress) : "memory");	// DCIMVAC

		if (nLength < DATA_CACHE_LINE_LENGTH_MIN)
		{
			break;
		}

		nAddress += DATA_CACHE_LINE_LENGTH_MIN;
		nLength  -= DATA_CACHE_LINE_LENGTH_MIN;
	}
}

void CleanDataCacheRange (u32 nAddress, u32 nLength)
{
	nLength += DATA_CACHE_LINE_LENGTH_MIN;

	while (1)
	{
		asm volatile ("mcr p15, 0, %0, c7, c10,  1" : : "r" (nAddress) : "memory");	// DCCMVAC

		if (nLength < DATA_CACHE_LINE_LENGTH_MIN)
		{
			break;
		}

		nAddress += DATA_CACHE_LINE_LENGTH_MIN;
		nLength  -= DATA_CACHE_LINE_LENGTH_MIN;
	}
}

void CleanAndInvalidateDataCacheRange (u32 nAddress, u32 nLength)
{
	nLength += DATA_CACHE_LINE_LENGTH_MIN;

	while (1)
	{
		asm volatile ("mcr p15, 0, %0, c7, c14,  1" : : "r" (nAddress) : "memory");	// DCCIMVAC

		if (nLength < DATA_CACHE_LINE_LENGTH_MIN)
		{
			break;
		}

		nAddress += DATA_CACHE_LINE_LENGTH_MIN;
		nLength  -= DATA_CACHE_LINE_LENGTH_MIN;
	}
}

#endif
