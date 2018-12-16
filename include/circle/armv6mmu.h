//
// armv6mmu.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2018  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_armv6mmu_h
#define _circle_armv6mmu_h

#include <circle/sysconfig.h>
#include <circle/macros.h>
#include <circle/types.h>

// NOTE: VMSAv6 and VMSAv7 are very similar for our purpose.
//	 The definitions in this file are used for both.

// Sizes
#define SMALL_PAGE_SIZE		0x1000
#define LARGE_PAGE_SIZE		0x10000
#define SECTION_SIZE		0x100000
#define SUPER_SECTION_SIZE	0x1000000

// Access permissions (AP[1:0])
#define AP_NO_ACCESS		0
#define AP_SYSTEM_ACCESS	1
#define AP_USER_RO_ACCESS	2
#define AP_ALL_ACCESS		3

// Access permissions extended (APX)
#define APX_RW_ACCESS		0
#define APX_RO_ACCESS		1

// Domains
#define DOMAIN_NO_ACCESS	0
#define DOMAIN_CLIENT		1
#define DOMAIN_MANAGER		3

#define ARMV6MMU_FAULT			0x00000		// no access

struct TARMV6MMU_LEVEL1_SECTION_DESCRIPTOR		// subpages disabled
{
	u32	Value10		: 2,		// set to 2
		BBit		: 1,		// bufferable bit
		CBit		: 1,		// cacheable bit
		XNBit		: 1,		// execute never bit
		Domain		: 4,		// 0..15
		IMPBit		: 1,		// implementation defined bit
		AP		: 2,		// access permissions
		TEX		: 3,		// memory type extension field
		APXBit		: 1,		// access permissions extended bit
		SBit		: 1,		// shareable bit
		NGBit		: 1,		// non-global bit
		Value0		: 1,		// set to 0
		SBZ		: 1,		// should be 0
		Base		: 12;		// base address [31:20]
}
PACKED;

#ifndef ARM_ALLOW_MULTI_CORE
#define ARMV6MMUL1SECTION_NORMAL	0x0040E		// outer and inner write back, no write allocate
#define ARMV6MMUL1SECTION_NORMAL_XN	0x0041E		//	+ execute never
#else
							// required for spin locks, TODO: shared pool
#define ARMV6MMUL1SECTION_NORMAL	0x1040E		//	+ shareable
#define ARMV6MMUL1SECTION_NORMAL_XN	0x1041E		//	+ shareable and execute never
#endif
#define ARMV6MMUL1SECTION_DEVICE	0x10416		// shared device
#define ARMV6MMUL1SECTION_COHERENT	0x10412		// strongly ordered

#define ARMV6MMUL1SECTIONBASE(addr)	(((addr) >> 20) & 0xFFF)
#define ARMV6MMUL1SECTIONPTR(base)	((void *) ((base) << 20))

struct TARMV6MMU_LEVEL1_COARSE_PAGE_TABLE_DESCRIPTOR	// subpages disabled
{
	u32	Value01		: 2,		// set to 1
		SBZ		: 3,		// should be 0
		Domain		: 4,		// 0..15
		IMPBit		: 1,		// implementation defined bit
		Base		: 22;		// coarse page table base address [31..10]
}
PACKED;

#define ARMV6MMUL1COARSEBASE(addr)	(((addr) >> 10) & 0x3FFFFF)
#define ARMV6MMUL1COARSEPTR(base)	((void *) ((base) << 10))

#define ARMV6MMU_LEVEL2_COARSE_PAGE_TABLE_SIZE	0x400

struct TARMV6MMU_LEVEL2_EXT_SMALL_PAGE_DESCRIPTOR	// subpages disabled
{
	u32	XNBit		: 1,		// execute never bit
		Value1		: 1,		// set to 1
		BBit		: 1,		// bufferable bit
		CBit		: 1,		// cacheable bit
		AP		: 2,		// access permissions
		TEX		: 3,		// memory type extension field
		APXBit		: 1,		// access permissions extended bit
		SBit		: 1,		// shareable bit
		NGBit		: 1,		// non-global bit
		Base		: 20;		// extended small page base address
}
PACKED;

#define ARMV6MMUL2SMALLPAGEBASE(addr)	(((addr) >> 12) & 0xFFFFF)
#define ARMV6MMUL2SMALLPAGEPTR(base)	((void *) ((base) << 12))

// TLB type register
#define ARM_TLB_TYPE_SEPARATE_TLBS	(1 << 0)

// (System) Control register
#define ARM_CONTROL_MMU			(1 << 0)
#define ARM_CONTROL_STRICT_ALIGNMENT	(1 << 1)
#define ARM_CONTROL_L1_CACHE		(1 << 2)
#define ARM_CONTROL_BRANCH_PREDICTION	(1 << 11)
#define ARM_CONTROL_L1_INSTRUCTION_CACHE (1 << 12)
#if RASPPI == 1
#define ARM_CONTROL_UNALIGNED_PERMITTED	(1 << 22)
#define ARM_CONTROL_EXTENDED_PAGE_TABLE	(1 << 23)
#endif

// Translation table base registers
#if RASPPI == 1
#define ARM_TTBR_INNER_CACHEABLE	(1 << 0)
#else
#define ARM_TTBR_INNER_NON_CACHEABLE	((0 << 6) | (0 << 0))
#define ARM_TTBR_INNER_WRITE_ALLOCATE	((1 << 6) | (0 << 0))
#define ARM_TTBR_INNER_WRITE_THROUGH	((0 << 6) | (1 << 0))
#define ARM_TTBR_INNER_WRITE_BACK	((1 << 6) | (1 << 0))
#endif

#define ARM_TTBR_USE_SHAREABLE_MEM	(1 << 1)

#define ARM_TTBR_OUTER_NON_CACHEABLE	(0 << 3)
#define ARM_TTBR_OUTER_WRITE_ALLOCATE	(1 << 3)
#define ARM_TTBR_OUTER_WRITE_THROUGH	(2 << 3)
#define ARM_TTBR_OUTER_WRITE_BACK	(3 << 3)

#if RASPPI != 1
#define ARM_TTBR_NOT_OUTER_SHAREABLE	(1 << 5)
#endif

//  Auxiliary Control register
#if RASPPI == 1
#define ARM_AUX_CONTROL_CACHE_SIZE	(1 << 6)	// restrict cache size to 16K (no page coloring)
#else
#define ARM_AUX_CONTROL_SMP		(1 << 6)
#endif

#endif
