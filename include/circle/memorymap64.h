//
// memorymap64.h
//
// Memory addresses and sizes (for AArch64)
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
#ifndef _circle_memorymap64_h
#define _circle_memorymap64_h

#ifndef _circle_memorymap_h
	#error Do not include this header file directly!
#endif

#ifndef MEGABYTE
	#define MEGABYTE	0x100000
#endif
#ifndef GIGABYTE
	#define GIGABYTE	0x40000000UL
#endif

#define CORES			4				// must be a power of 2

#define MEM_SIZE		(512 * MEGABYTE)		// default size
#define GPU_MEM_SIZE		(64 * MEGABYTE)			// set in config.txt
#define ARM_MEM_SIZE		(MEM_SIZE - GPU_MEM_SIZE)	// normally overwritten

#define PAGE_SIZE		0x10000				// page size used by us

#define EXCEPTION_STACK_SIZE	0x8000
#define PAGE_RESERVE		(16 * MEGABYTE)

#define MEM_KERNEL_START	0x80000					// main code starts here
#define MEM_KERNEL_END		(MEM_KERNEL_START + KERNEL_MAX_SIZE)
#define MEM_KERNEL_STACK	(MEM_KERNEL_END + KERNEL_STACK_SIZE)	// expands down
#define MEM_EXCEPTION_STACK	(MEM_KERNEL_STACK + KERNEL_STACK_SIZE * (CORES-1) + EXCEPTION_STACK_SIZE)
#define MEM_EXCEPTION_STACK_END	(MEM_EXCEPTION_STACK + EXCEPTION_STACK_SIZE * (CORES-1))

#if RASPPI <= 3
// coherent memory region (1 MB)
#define MEM_COHERENT_REGION	((MEM_EXCEPTION_STACK_END + 2*MEGABYTE) & ~(MEGABYTE-1))

#define MEM_HEAP_START		(MEM_COHERENT_REGION + MEGABYTE)
#else
// coherent memory region (4 MB)
#define MEM_COHERENT_REGION	((MEM_EXCEPTION_STACK_END + 2*MEGABYTE) & ~(MEGABYTE-1))

#define MEM_HEAP_START		(MEM_COHERENT_REGION + 4*MEGABYTE)
#endif

#if RASPPI >= 4
// high memory region (memory >= 3 GB is not safe to be DMA-able and is not used)
#define MEM_HIGHMEM_START		GIGABYTE
#define MEM_HIGHMEM_END			(3 * GIGABYTE - 1)

// PCIe memory range (outbound)
#define MEM_PCIE_RANGE_START		0x600000000UL
#define MEM_PCIE_RANGE_SIZE		0x4000000UL
#define MEM_PCIE_RANGE_PCIE_START	0xF8000000UL		// mapping on PCIe side
#define MEM_PCIE_RANGE_START_VIRTUAL	MEM_PCIE_RANGE_START
#define MEM_PCIE_RANGE_END_VIRTUAL	(MEM_PCIE_RANGE_START_VIRTUAL + MEM_PCIE_RANGE_SIZE - 1UL)

// PCIe memory range (inbound)
#define MEM_PCIE_DMA_RANGE_START	0UL
#define MEM_PCIE_DMA_RANGE_SIZE		0x100000000UL
#define MEM_PCIE_DMA_RANGE_PCIE_START	0UL			// mapping on PCIe side
#endif

#endif
