//
// sysinit.cpp
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
#include <circle/startup.h>
#include <circle/memio.h>
#include <circle/bcm2835.h>
#include <circle/bcm2836.h>
#include <circle/machineinfo.h>
#include <circle/synchronize.h>
#include <circle/sysconfig.h>
#include <circle/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void *__dso_handle;

void __aeabi_atexit (void *pThis, void (*pFunc)(void *pThis), void *pHandle)
{
	// TODO
}

void halt (void)
{
#ifdef ARM_ALLOW_MULTI_CORE
	static volatile boolean s_bCoreHalted[CORES] = {FALSE};

	u32 nMPIDR;
	asm volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r" (nMPIDR));
	unsigned nCore = nMPIDR & (CORES-1);

	// core 0 must not halt until all secondary cores have been halted
	if (nCore == 0)
	{
		unsigned nSecCore = 1;
		while (nSecCore < CORES)
		{
			DataMemBarrier ();
			if (!s_bCoreHalted[nSecCore])
			{
				DataSyncBarrier ();
				asm volatile ("wfi");

				nSecCore = 1;
			}
			else
			{
				nSecCore++;
			}
		}
	}

	s_bCoreHalted[nCore] = TRUE;
	DataMemBarrier ();
#endif

	DisableInterrupts ();
	
	for (;;)
	{
#if RASPPI != 1
		DataSyncBarrier ();
		asm volatile ("wfi");
#endif
	}
}

void reboot (void)					// by PlutoniumBob@raspi-forum
{
	PeripheralEntry ();

	write32 (ARM_PM_WDOG, ARM_PM_PASSWD | 1);	// set some timeout

#define PM_RSTC_WRCFG_FULL_RESET	0x20
	write32 (ARM_PM_RSTC, ARM_PM_PASSWD | PM_RSTC_WRCFG_FULL_RESET);

	for (;;);					// wait for reset
}

static void vfpinit (void)
{
	// Coprocessor Access Control Register
	unsigned nCACR;
	__asm volatile ("mrc p15, 0, %0, c1, c0, 2" : "=r" (nCACR));
	nCACR |= 3 << 20;	// cp10 (single precision)
	nCACR |= 3 << 22;	// cp11 (double precision)
	__asm volatile ("mcr p15, 0, %0, c1, c0, 2" : : "r" (nCACR));
	InstructionMemBarrier ();

#define VFP_FPEXC_EN	(1 << 30)
	__asm volatile ("fmxr fpexc, %0" : : "r" (VFP_FPEXC_EN));

	__asm volatile ("fmxr fpscr, %0" : : "r" (0));
}

void sysinit (void)
{
#if RASPPI != 1
#ifndef USE_RPI_STUB_AT
	// L1 data cache may contain random entries after reset, delete them
	InvalidateDataCacheL1Only ();
#endif
#ifndef ARM_ALLOW_MULTI_CORE
	// put all secondary cores to sleep
	for (unsigned nCore = 1; nCore < CORES; nCore++)
	{
		write32 (ARM_LOCAL_MAILBOX3_SET0 + 0x10 * nCore, (u32) &_start_secondary);
	}
#endif
#endif

	vfpinit ();

	// clear BSS
	extern unsigned char __bss_start;
	extern unsigned char _end;
	for (unsigned char *pBSS = &__bss_start; pBSS < &_end; pBSS++)
	{
		*pBSS = 0;
	}

	CMachineInfo MachineInfo;

	// call construtors of static objects
	extern void (*__init_start) (void);
	extern void (*__init_end) (void);
	for (void (**pFunc) (void) = &__init_start; pFunc < &__init_end; pFunc++)
	{
		(**pFunc) ();
	}

	extern int main (void);
	if (main () == EXIT_REBOOT)
	{
		reboot ();
	}

	halt ();
}

#ifdef ARM_ALLOW_MULTI_CORE

void sysinit_secondary (void)
{
	// L1 data cache may contain random entries after reset, delete them
	InvalidateDataCacheL1Only ();

	vfpinit ();

	main_secondary ();

	halt ();
}

#endif

#ifdef __cplusplus
}
#endif
