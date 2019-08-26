//
// armv7lpae.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_armv7lpae_h
#define _circle_armv7lpae_h

#include <circle/macros.h>
#include <circle/types.h>

#define MEGABYTE			0x100000

#define ARMV7LPAE_TABLE_ENTRIES		512

// Level 1
struct TARMV7LPAE_LEVEL1_TABLE_DESCRIPTOR
{
	u64	Value11		: 2,		// set to 3
		Ignored1	: 10,		// set to 0
		TableAddress	: 28,		// table base address [39:12]
		Unknown		: 12,		// set to 0
		Ignored2	: 7,		// set to 0
		PXNTable	: 1,		// set to 0
		XNTable		: 1,		// set to 0
		APTable		: 2,
#define AP_TABLE_ALL_ACCESS		0
		NSTable		: 1;		// ignored, set to 0
}
PACKED;

#define ARMV7LPAEL1TABLEADDR(addr)	(((addr) >> 12) & 0xFFFFFFF)
#define ARMV7LPAEL1TABLEPTR(table)	((void *) ((table) << 12))


// Level 2
struct TARMV7LPAE_LEVEL2_BLOCK_DESCRIPTOR
{
	u64	Value01		: 2,		// set to 1
		//LowerAttributes	: 10,
			AttrIndx	: 3,	// [2:0], see MAIR
			NS		: 1,	// ignored, set to 0
			AP		: 2,	// [2:1]
#define ATTRIB_AP_RW_PL1		0
#define ATTRIB_AP_RW_ALL		1
#define ATTRIB_AP_RO_PL1		2
#define ATTRIB_AP_RO_ALL		3
			SH		: 2,	// [1:0]
#define ATTRIB_SH_NON_SHAREABLE		0
#define ATTRIB_SH_OUTER_SHAREABLE	2
#define ATTRIB_SH_INNER_SHAREABLE	3
			AF		: 1,	// set to 1, will fault otherwise
			nG		: 1,	// set to 0
		Unknown1	: 9,		// set to 0
		OutputAddress	: 19,		// [39:21]
		Unknown2	: 12,		// set to 0
		//UpperAttributes	: 12
			Continous	: 1,	// set to 0
			PXN		: 1,	// set to 0, 1 for device memory
			XN		: 1,	// set to 1
			Ignored		: 9;	// set to 0
}
PACKED;

#define ARMV7LPAE_LEVEL2_BLOCK_SIZE	(2 * MEGABYTE)
#define ARMV7LPAEL2BLOCKADDR(addr)	(((addr) >> 21) & 0x7FFFF)
#define ARMV7LPAEL2BLOCKPTR(block)	((void *) ((table) << 21))


// System registers
#define LPAE_TTBCR_EAE		(1 << 31)		// TTBCR
#define LPAE_TTBCR_EPD1		(1 << 23)
#define LPAE_TTBCR_A1		(1 << 22)
#define LPAE_TTBCR_SH0__SHIFT	12
#define LPAE_TTBCR_SH0__MASK	(3 << 12)
#define LPAE_TTBCR_ORGN0__SHIFT	10
#define LPAE_TTBCR_ORGN0__MASK	(3 << 10)
	#define LPAE_TTBCR_ORGN0_WR_BACK_ALLOCATE	1
	#define LPAE_TTBCR_ORGN0_WR_BACK		3
#define LPAE_TTBCR_IRGN0__SHIFT	8
#define LPAE_TTBCR_IRGN0__MASK	(3 << 8)
	#define LPAE_TTBCR_IRGN0_WR_BACK_ALLOCATE	1
	#define LPAE_TTBCR_IRGN0_WR_BACK		3
#define LPAE_TTBCR_EPD0		(1 << 7)
#define LPAE_TTBCR_T0SZ__MASK	(7 << 0)
	#define LPAE_TTBCR_T0SZ_4GB			0

#define LPAE_MAIR_NORMAL	0xFF			// MAIRn
#define LPAE_MAIR_DEVICE	0x04
#define LPAE_MAIR_COHERENT	0x00

#endif
