//
// armv8mmu.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2019  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_armv8mmu_h
#define _circle_armv8mmu_h

#include <circle/macros.h>
#include <circle/types.h>

// These declarations are intended to be used with granule size 64KB and
// non-secure EL1 stage 1 translation only. IPA (= PA) size is 32 bits (4GB).

#define MEGABYTE			0x100000

#define ARMV8MMU_GRANULE_SIZE		0x10000		// 64KB
#define ARMV8MMU_TABLE_ENTRIES		8192

// Level 2
struct TARMV8MMU_LEVEL2_TABLE_DESCRIPTOR
{
	u64	Value11		: 2,		// set to 3
		Ignored1	: 14,		// set to 0
		TableAddress	: 32,		// table base address [47:16]
		Reserved0	: 4,		// set to 0
		Ignored2	: 7,		// set to 0
		PXNTable	: 1,		// set to 0
		UXNTable	: 1,		// set to 0
		APTable		: 2,
#define AP_TABLE_ALL_ACCESS		0
		NSTable		: 1;		// RES0, set to 0
}
PACKED;

#define ARMV8MMUL2TABLEADDR(addr)	(((addr) >> 16) & 0xFFFFFFFF)
#define ARMV8MMUL2TABLEPTR(table)	((void *) ((table) << 16))

struct TARMV8MMU_LEVEL2_BLOCK_DESCRIPTOR
{
	u64	Value01		: 2,		// set to 1
		//LowerAttributes	: 10,
			AttrIndx	: 3,	// [2:0], see MAIR_EL1
			NS		: 1,	// RES0, set to 0
			AP		: 2,	// [2:1]
#define ATTRIB_AP_RW_EL1		0
#define ATTRIB_AP_RW_ALL		1
#define ATTRIB_AP_RO_EL1		2
#define ATTRIB_AP_RO_ALL		3
			SH		: 2,	// [1:0]
#define ATTRIB_SH_NON_SHAREABLE		0
#define ATTRIB_SH_OUTER_SHAREABLE	2
#define ATTRIB_SH_INNER_SHAREABLE	3
			AF		: 1,	// set to 1, will fault otherwise
			nG		: 1,	// set to 0
		Reserved0_1	: 17,		// set to 0
		OutputAddress	: 19,		// [47:29]
		Reserved0_2	: 4,		// set to 0
		//UpperAttributes	: 12
			Continous	: 1,	// set to 0
			PXN		: 1,	// set to 0, 1 for device memory
			UXN		: 1,	// set to 1
			Ignored		: 9;	// set to 0
}
PACKED;

#define ARMV8MMU_LEVEL2_BLOCK_SIZE	(512 * MEGABYTE)
#define ARMV8MMUL2BLOCKADDR(addr)	(((addr) >> 29) & 0x7FFFF)
#define ARMV8MMUL2BLOCKPTR(block)	((void *) ((table) << 29))

struct TARMV8MMU_LEVEL2_INVALID_DESCRIPTOR
{
	u64	Value0		: 1,		// set to 0
		Ignored		: 63;
}
PACKED;

union TARMV8MMU_LEVEL2_DESCRIPTOR
{
	TARMV8MMU_LEVEL2_TABLE_DESCRIPTOR	Table;
	TARMV8MMU_LEVEL2_BLOCK_DESCRIPTOR	Block;
	TARMV8MMU_LEVEL2_INVALID_DESCRIPTOR	Invalid;
}
PACKED;


// Level 3
struct TARMV8MMU_LEVEL3_PAGE_DESCRIPTOR
{
	u64	Value11		: 2,		// set to 3
		//LowerAttributes	: 10,
			AttrIndx	: 3,	// [2:0], see MAIR_EL1
			NS		: 1,	// RES0, set to 0
			AP		: 2,	// [2:1]
			SH		: 2,	// [1:0]
			AF		: 1,	// set to 1, will fault otherwise
			nG		: 1,	// set to 0
		Reserved0_1	: 4,		// set to 0
		OutputAddress	: 32,		// [47:16]
		Reserved0_2	: 4,		// set to 0
		//UpperAttributes	: 12
			Continous	: 1,	// set to 0
			PXN		: 1,	// set to 0, 1 for device memory
			UXN		: 1,	// set to 1
			Ignored		: 9	// set to 0
		;
}
PACKED;

#define ARMV8MMU_LEVEL3_PAGE_SIZE	0x10000
#define ARMV8MMUL3PAGEADDR(addr)	(((addr) >> 16) & 0xFFFFFFFF)
#define ARMV8MMUL3PAGEPTR(page)		((void *) ((page) << 16))

struct TARMV8MMU_LEVEL3_INVALID_DESCRIPTOR
{
	u64	Value0		: 1,		// set to 0
		Ignored		: 63;
}
PACKED;

union TARMV8MMU_LEVEL3_DESCRIPTOR
{
	TARMV8MMU_LEVEL3_PAGE_DESCRIPTOR	Page;
	TARMV8MMU_LEVEL3_INVALID_DESCRIPTOR	Invalid;
}
PACKED;


// System registers
#define SCTLR_EL1_WXN		(1 << 19)		// SCTLR_EL1
#define SCTLR_EL1_I		(1 << 12)
#define SCTLR_EL1_C		(1 << 2)
#define SCTLR_EL1_A		(1 << 1)
#define SCTLR_EL1_M		(1 << 0)

#define TCR_EL1_IPS__SHIFT	32			// TCR_EL1
#define TCR_EL1_IPS__MASK	(7UL << 32)
	#define TCR_EL1_IPS_4GB		0UL
	#define TCR_EL1_IPS_64GB	1UL
#define TCR_EL1_EPD1		(1 << 23)
#define TCR_EL1_A1		(1 << 22)
#define TCR_EL1_TG0__SHIFT	14
#define TCR_EL1_TG0__MASK	(3 << 14)
	#define TCR_EL1_TG0_64KB	1
#define TCR_EL1_SH0__SHIFT	12
#define TCR_EL1_SH0__MASK	(3 << 12)
	#define TCR_EL1_SH0_INNER	3
#define TCR_EL1_ORGN0__SHIFT	10
#define TCR_EL1_ORGN0__MASK	(3 << 10)
	#define TCR_EL1_ORGN0_WR_BACK_ALLOCATE	1
	#define TCR_EL1_ORGN0_WR_BACK		3
#define TCR_EL1_IRGN0__SHIFT	8
#define TCR_EL1_IRGN0__MASK	(3 << 8)
	#define TCR_EL1_IRGN0_WR_BACK_ALLOCATE	1
	#define TCR_EL1_IRGN0_WR_BACK		3
#define TCR_EL1_EPD0		(1 << 7)
#define TCR_EL1_T0SZ__SHIFT	0
#define TCR_EL1_T0SZ__MASK	(0x3F << 0)
	#define TCR_EL1_T0SZ_4GB	32
	#define TCR_EL1_T0SZ_64GB	28

#endif
