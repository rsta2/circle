//
// synchronize.h
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
#ifndef _circle_synchronize_h
#define _circle_synchronize_h

#if AARCH == 64
	#include <circle/synchronize64.h>
#else

#include <circle/macros.h>
#include <circle/types.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// Execution levels
//
#define TASK_LEVEL		0		// IRQs and FIQs enabled
#define IRQ_LEVEL		1		// IRQs disabled, FIQs enabled
#define FIQ_LEVEL		2		// IRQs and FIQs disabled

unsigned CurrentExecutionLevel (void);

//
// Interrupt control
//
#define	EnableIRQs()		asm volatile ("cpsie i")
#define	DisableIRQs()		asm volatile ("cpsid i")
#define	EnableInterrupts()	EnableIRQs()			// deprecated
#define	DisableInterrupts()	DisableIRQs()			// deprecated

#define	EnableFIQs()		asm volatile ("cpsie f")
#define	DisableFIQs()		asm volatile ("cpsid f")

// EnterCritical() can be nested with same or increasing nTargetLevel
void EnterCritical (unsigned nTargetLevel = IRQ_LEVEL);
void LeaveCritical (void);

#if RASPPI == 1

//
// Cache control
//
#define DATA_CACHE_LINE_LENGTH_MIN	32	// from CTR
#define DATA_CACHE_LINE_LENGTH_MAX	32

#define InvalidateInstructionCache()	\
				asm volatile ("mcr p15, 0, %0, c7, c5,  0" : : "r" (0) : "memory")
#define FlushPrefetchBuffer()	asm volatile ("mcr p15, 0, %0, c7, c5,  4" : : "r" (0) : "memory")
#define FlushBranchTargetCache()	\
				asm volatile ("mcr p15, 0, %0, c7, c5,  6" : : "r" (0) : "memory")

// NOTE: Data cache operations include a DataSyncBarrier
#define InvalidateDataCache()	asm volatile ("mcr p15, 0, %0, c7, c6,  0\n" \
					      "mcr p15, 0, %0, c7, c10, 4\n" : : "r" (0) : "memory")
#define CleanDataCache()	asm volatile ("mcr p15, 0, %0, c7, c10, 0\n" \
					      "mcr p15, 0, %0, c7, c10, 4\n" : : "r" (0) : "memory")

void CleanAndInvalidateDataCacheRange (u32 nAddress, u32 nLength) MAXOPT;

void SyncDataAndInstructionCache (void);

//
// Barriers
//
#define DataSyncBarrier()	asm volatile ("mcr p15, 0, %0, c7, c10, 4" : : "r" (0) : "memory")
#define DataMemBarrier() 	asm volatile ("mcr p15, 0, %0, c7, c10, 5" : : "r" (0) : "memory")

#define InstructionSyncBarrier() FlushPrefetchBuffer()
#define InstructionMemBarrier()	FlushPrefetchBuffer()

// According to the "BCM2835 ARM Peripherals" document pg. 7 the BCM2835
// requires to insert barriers before writing and after reading to/from
// a peripheral for in-order processing of data transferred on the AXI bus.
#define PeripheralEntry()	DataSyncBarrier()
#define PeripheralExit()	DataMemBarrier()

#else

//
// Cache control
//
#define DATA_CACHE_LINE_LENGTH_MIN	64	// from CTR
#define DATA_CACHE_LINE_LENGTH_MAX	64

#define InvalidateInstructionCache()	\
				asm volatile ("mcr p15, 0, %0, c7, c5,  0" : : "r" (0) : "memory")
#define FlushPrefetchBuffer()	asm volatile ("isb" ::: "memory")
#define FlushBranchTargetCache()	\
				asm volatile ("mcr p15, 0, %0, c7, c5,  6" : : "r" (0) : "memory")

// cache-v7.S
//
// NOTE: Data cache operations include a DataSyncBarrier
void InvalidateDataCacheL1Only (void);
void InvalidateDataCache (void);
void CleanDataCache (void);
void CleanAndInvalidateDataCacheRange (u32 nAddress, u32 nLength);

void SyncDataAndInstructionCache (void);

//
// Barriers
//
#define DataSyncBarrier()	asm volatile ("dsb" ::: "memory")
#define DataMemBarrier() 	asm volatile ("dmb" ::: "memory")

#define InstructionSyncBarrier() asm volatile ("isb" ::: "memory")
#define InstructionMemBarrier()	asm volatile ("isb" ::: "memory")

#define PeripheralEntry()	((void) 0)	// ignored here
#define PeripheralExit()	((void) 0)

//
// Wait for interrupt and event
//
#define WaitForInterrupt()	asm volatile ("wfi")
#define WaitForEvent()		asm volatile ("wfe")
#define SendEvent()		asm volatile ("sev")

#endif

#define CompilerBarrier()	asm volatile ("" ::: "memory")

//
// Cache alignment
//
#define CACHE_ALIGN			ALIGN (DATA_CACHE_LINE_LENGTH_MAX)

#define CACHE_ALIGN_SIZE(type, num)	(((  ((num)*sizeof (type) - 1)		\
					   | (DATA_CACHE_LINE_LENGTH_MAX-1)	\
					  ) + 1) / sizeof (type))

#define IS_CACHE_ALIGNED(ptr, size)	(   ((uintptr) (ptr) & (DATA_CACHE_LINE_LENGTH_MAX-1)) == 0 \
					 && ((size) & (DATA_CACHE_LINE_LENGTH_MAX-1)) == 0)

#define DMA_BUFFER(type, name, num)	type name[CACHE_ALIGN_SIZE (type, num)] CACHE_ALIGN

#ifdef __cplusplus
}
#endif

#endif

#endif
