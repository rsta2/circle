//
// sysinit.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2023  R. Stange <rsta2@o2online.de>
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
#include <circle/memory.h>
#include <circle/chainboot.h>
#include <circle/qemu.h>
#include <circle/synchronize.h>
#include <circle/sysconfig.h>
#include <circle/memorymap.h>
#include <circle/version.h>
#include <circle/string.h>
#include <circle/macros.h>
#include <circle/util.h>
#include <circle/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void *__dso_handle WEAK;

void __aeabi_atexit (void *pThis, void (*pFunc)(void *pThis), void *pHandle) WEAK;

void __aeabi_atexit (void *pThis, void (*pFunc)(void *pThis), void *pHandle)
{
	// TODO
}

#if AARCH == 64 || defined (__clang__)

void __cxa_atexit (void *pThis, void (*pFunc)(void *pThis), void *pHandle) WEAK;

void __cxa_atexit (void *pThis, void (*pFunc)(void *pThis), void *pHandle)
{
	// TODO
}

#endif

#if STDLIB_SUPPORT >= 2

void __sync_synchronize (void)
{
	DataSyncBarrier ();
}

#endif

#if STDLIB_SUPPORT == 1

int *__errno (void)
{
	static int errno = 0;

	return &errno;
}

#endif

static int s_nExitStatus = EXIT_STATUS_SUCCESS;

void set_qemu_exit_status (int nStatus)
{
	s_nExitStatus = nStatus;
}

void halt (void)
{
#ifdef ARM_ALLOW_MULTI_CORE
	static volatile boolean s_bCoreHalted[CORES] = {FALSE};

#if AARCH == 32
	u32 nMPIDR;
	asm volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r" (nMPIDR));
#else
	u64 nMPIDR;
	asm volatile ("mrs %0, mpidr_el1" : "=r" (nMPIDR));
#endif
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

	DisableIRQs ();
#ifndef USE_RPI_STUB_AT
	DisableFIQs ();
#endif

#ifdef LEAVE_QEMU_ON_HALT
#ifdef ARM_ALLOW_MULTI_CORE
	if (nCore == 0)
#endif
	{
		// exit QEMU using the ARM semihosting (aka "Angel") interface
		SemihostingExit (s_nExitStatus);
	}
#endif

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

#if AARCH == 32

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

#define VFP_FPSCR_FZ	(1 << 24)	// enable Flush-to-zero mode
#define VFP_FPSCR_DN	(1 << 25)	// enable Default NaN mode
	__asm volatile ("fmxr fpscr, %0" : : "r" (VFP_FPSCR_FZ | VFP_FPSCR_DN));
}

#endif

char circle_version_string[10];

void sysinit (void)
{
	EnableFIQs ();		// go to IRQ_LEVEL, EnterCritical() will not work otherwise
	EnableIRQs ();		// go to TASK_LEVEL

#if AARCH == 32
#if RASPPI != 1
#ifndef USE_RPI_STUB_AT
	// L1 data cache may contain random entries after reset, delete them
	InvalidateDataCacheL1Only ();
#endif
#endif

	vfpinit ();

#if RASPPI >= 4
	// generate 1 MHz clock for timer from 54 MHz oscillator
	write32 (ARM_LOCAL_PRESCALER, 39768216U);
#endif
#endif

	// clear BSS
	extern unsigned char __bss_start;
	extern unsigned char _end;
	memset (&__bss_start, 0, &_end - &__bss_start);

	// halt, if KERNEL_MAX_SIZE is not properly set
	// cannot inform the user here
	if (MEM_KERNEL_END < reinterpret_cast<uintptr> (&_end))
	{
		halt ();
	}

	CMachineInfo MachineInfo;

	CMemorySystem Memory;

#if RASPPI >= 4
	MachineInfo.FetchDTB ();
#endif

	// set circle_version_string[]
	CString Version;
	if (CIRCLE_PATCH_VERSION)
	{
		Version.Format ("%d.%d.%d", CIRCLE_MAJOR_VERSION, CIRCLE_MINOR_VERSION,
			        CIRCLE_PATCH_VERSION);
	}
	else if (CIRCLE_MINOR_VERSION)
	{
		Version.Format ("%d.%d", CIRCLE_MAJOR_VERSION, CIRCLE_MINOR_VERSION);
	}
	else
	{
		Version.Format ("%d", CIRCLE_MAJOR_VERSION);
	}

	strcpy (circle_version_string, Version);

	// call constructors of static objects
	extern void (*__init_start) (void);
	extern void (*__init_end) (void);
	for (void (**pFunc) (void) = &__init_start; pFunc < &__init_end; pFunc++)
	{
		(**pFunc) ();
	}

	extern int MAINPROC (void);
	if (MAINPROC () == EXIT_REBOOT)
	{
		if (IsChainBootEnabled ())
		{
			Memory.Destructor ();

			DisableFIQs ();

			DoChainBoot ();
		}

		reboot ();
	}

	halt ();
}

#ifdef ARM_ALLOW_MULTI_CORE

void sysinit_secondary (void)
{
	EnableFIQs ();		// go to IRQ_LEVEL, EnterCritical() will not work otherwise
	EnableIRQs ();		// go to TASK_LEVEL

#if AARCH == 32
	// L1 data cache may contain random entries after reset, delete them
	InvalidateDataCacheL1Only ();

	vfpinit ();
#endif

	main_secondary ();

	halt ();
}

#endif

#ifdef __cplusplus
}
#endif
