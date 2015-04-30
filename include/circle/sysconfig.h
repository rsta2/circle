//
// sysconfig.h
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
#ifndef _sysconfig_h
#define _sysconfig_h

// memory addresses and sizes
#define MEGABYTE		0x100000

#define MEM_SIZE		(256 * MEGABYTE)		// default size
#define GPU_MEM_SIZE		(64 * MEGABYTE)			// set in config.txt
#define ARM_MEM_SIZE		(MEM_SIZE - GPU_MEM_SIZE)	// normally overwritten

#define PAGE_SIZE		4096				// page size used by us

#define KERNEL_MAX_SIZE		(2 * MEGABYTE)			// all sizes must be a multiple of 16K
#define KERNEL_STACK_SIZE	0x20000
#define EXCEPTION_STACK_SIZE	0x8000
#define PAGE_TABLE1_SIZE	0x4000
#define PAGE_RESERVE		(4 * MEGABYTE)

#define MEM_KERNEL_START	0x8000
#define MEM_KERNEL_END		(MEM_KERNEL_START + KERNEL_MAX_SIZE)
#define MEM_KERNEL_STACK	(MEM_KERNEL_END + KERNEL_STACK_SIZE)		// expands down
#if RASPPI == 1
#define MEM_ABORT_STACK		(MEM_KERNEL_STACK + EXCEPTION_STACK_SIZE)	// expands down
#define MEM_IRQ_STACK		(MEM_ABORT_STACK + EXCEPTION_STACK_SIZE)	// expands down
#define MEM_PAGE_TABLE1		MEM_IRQ_STACK				// must be 16K aligned
#else
#define CORES			4					// must be a power of 2
#define MEM_ABORT_STACK		(MEM_KERNEL_STACK + KERNEL_STACK_SIZE * (CORES-1) + EXCEPTION_STACK_SIZE)
#define MEM_IRQ_STACK		(MEM_ABORT_STACK + EXCEPTION_STACK_SIZE * (CORES-1) + EXCEPTION_STACK_SIZE)
#define MEM_PAGE_TABLE1		(MEM_IRQ_STACK + EXCEPTION_STACK_SIZE * (CORES-1))
#endif

#define MEM_HEAP_START		0x400000

// system options
#if RASPPI == 1			// valid on Raspberry Pi 1 only
#define ARM_STRICT_ALIGNMENT
#define GPU_L2_CACHE_ENABLED
#endif

#if RASPPI == 2			// valid on Raspberry Pi 2 only
#define ARM_ALLOW_MULTI_CORE	// slower on single core if defined
#endif

#define KERNEL_TIMERS		20

// default keyboard map (enable only one)
//#define DEFAULT_KEYMAP_UK
#define DEFAULT_KEYMAP_DE
//#define DEFAULT_KEYMAP_ES

#endif
