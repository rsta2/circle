//
// pagetable.cpp
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
#include <circle/pagetable.h>
#include <circle/alloc.h>
#include <circle/sysconfig.h>
#include <circle/synchronize.h>
#include <circle/util.h>
#include <assert.h>

#if RASPPI == 1
#define SDRAM_SIZE_MBYTE	512

#define TTBR_MODE	(  ARM_TTBR_INNER_CACHEABLE		\
			 | ARM_TTBR_OUTER_NON_CACHEABLE)
#else
#define SDRAM_SIZE_MBYTE	1024

#define TTBR_MODE	(  ARM_TTBR_INNER_WRITE_BACK		\
			 | ARM_TTBR_OUTER_WRITE_BACK)
#endif

CPageTable::CPageTable (void)
:	m_bTableAllocated (FALSE),
	m_pTable ((TARMV6MMU_LEVEL1_SECTION_DESCRIPTOR *) MEM_PAGE_TABLE1)
{
	assert (((u32) m_pTable & 0x3FFF) == 0);

	for (unsigned nEntry = 0; nEntry < 4096; nEntry++)
	{
		u32 nBaseAddress = MEGABYTE * nEntry;
		
		TARMV6MMU_LEVEL1_SECTION_DESCRIPTOR *pEntry = &m_pTable[nEntry];

		// shared device
		pEntry->Value10	= 2;
		pEntry->BBit    = 1;
		pEntry->CBit    = 0;
		pEntry->XNBit   = 0;
		pEntry->Domain	= 0;
		pEntry->IMPBit	= 0;
		pEntry->AP	= AP_SYSTEM_ACCESS;
		pEntry->TEX     = 0;
		pEntry->APXBit	= APX_RW_ACCESS;
		pEntry->SBit    = 1;
		pEntry->NGBit	= 0;
		pEntry->Value0	= 0;
		pEntry->SBZ	= 0;
		pEntry->Base	= ARMV6MMUL1SECTIONBASE (nBaseAddress);

		if (nEntry >= SDRAM_SIZE_MBYTE)
		{
			pEntry->XNBit = 1;
		}
	}

	CleanDataCache ();
}

CPageTable::CPageTable (u32 nMemSize)
:	m_bTableAllocated (TRUE),
	m_pTable ((TARMV6MMU_LEVEL1_SECTION_DESCRIPTOR *) palloc ())
{
	assert (m_pTable != 0);
	assert (((u32) m_pTable & 0xFFF) == 0);

	for (unsigned nEntry = 0; nEntry < SDRAM_SIZE_MBYTE; nEntry++)
	{
		u32 nBaseAddress = MEGABYTE * nEntry;

		TARMV6MMU_LEVEL1_SECTION_DESCRIPTOR *pEntry = &m_pTable[nEntry];

		// outer and inner write back, no write allocate
		pEntry->Value10	= 2;
		pEntry->BBit	= 1;
		pEntry->CBit	= 1;
		pEntry->XNBit	= 0;
		pEntry->Domain	= 0;
		pEntry->IMPBit	= 0;
		pEntry->AP	= AP_SYSTEM_ACCESS;
		pEntry->TEX	= 0;
		pEntry->APXBit	= APX_RW_ACCESS;
#ifndef ARM_ALLOW_MULTI_CORE
		pEntry->SBit	= 0;
#else
		pEntry->SBit	= 1;		// required for spin locks, TODO: shared pool
#endif
		pEntry->NGBit	= 0;
		pEntry->Value0	= 0;
		pEntry->SBZ	= 0;
		pEntry->Base	= ARMV6MMUL1SECTIONBASE (nBaseAddress);

		extern u8 _etext;
		if (nBaseAddress >= (u32) &_etext)
		{
			pEntry->XNBit = 1;

			if (nBaseAddress >= nMemSize)
			{
				// shared device
				pEntry->BBit  = 1;
				pEntry->CBit  = 0;
				pEntry->TEX   = 0;
				pEntry->SBit  = 1;
			}
			else if (nBaseAddress == MEM_COHERENT_REGION)
			{
				// strongly ordered
				pEntry->BBit  = 0;
				pEntry->CBit  = 0;
				pEntry->TEX   = 0;
				pEntry->SBit  = 1;
			}
		}
	}

	CleanDataCache ();
}

CPageTable::~CPageTable (void)
{
	if (m_bTableAllocated)
	{
		pfree (m_pTable);
		m_pTable = 0;
	}
}

u32 CPageTable::GetBaseAddress (void) const
{
	return (u32) m_pTable | TTBR_MODE;
}
