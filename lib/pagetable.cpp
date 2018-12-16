//
// pagetable.cpp
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
#include <circle/pagetable.h>
#include <circle/armv6mmu.h>
#include <circle/sysconfig.h>
#include <circle/bcm2835.h>
#include <circle/bcm2836.h>
#include <circle/synchronize.h>
#include <assert.h>

#if RASPPI == 1
#define MEM_TOTAL_END	ARM_IO_END

#define TTBR_MODE	(  ARM_TTBR_INNER_CACHEABLE		\
			 | ARM_TTBR_OUTER_NON_CACHEABLE)
#else
#define MEM_TOTAL_END	ARM_LOCAL_END

#define TTBR_MODE	(  ARM_TTBR_INNER_WRITE_BACK		\
			 | ARM_TTBR_OUTER_WRITE_BACK)
#endif

CPageTable::CPageTable (u32 nMemSize)
:	m_pTable ((u32 *) MEM_PAGE_TABLE1)
{
	assert (((u32) m_pTable & 0x3FFF) == 0);

	for (unsigned nEntry = 0; nEntry < 4096; nEntry++)
	{
		u32 nBaseAddress = MEGABYTE * nEntry;

		u32 nAttributes = ARMV6MMU_FAULT;

		extern u8 _etext;
		if (nBaseAddress < (u32) &_etext)
		{
			nAttributes = ARMV6MMUL1SECTION_NORMAL;
		}
		else if (nBaseAddress == MEM_COHERENT_REGION)
		{
			nAttributes = ARMV6MMUL1SECTION_COHERENT;
		}
		else if (nBaseAddress < nMemSize)
		{
			nAttributes = ARMV6MMUL1SECTION_NORMAL_XN;
		}
		else if (nBaseAddress < MEM_TOTAL_END)
		{
			nAttributes = ARMV6MMUL1SECTION_DEVICE;
		}

		m_pTable[nEntry] = nBaseAddress | nAttributes;
	}

	CleanDataCache ();
}

CPageTable::~CPageTable (void)
{
}

u32 CPageTable::GetBaseAddress (void) const
{
	return (u32) m_pTable | TTBR_MODE;
}
