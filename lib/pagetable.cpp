//
// pagetable.cpp
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
#include <circle/pagetable.h>
#if RASPPI <= 3
	#include <circle/armv6mmu.h>
#else
	#include <circle/armv7lpae.h>
#endif
#include <circle/sysconfig.h>
#include <circle/bcm2835.h>
#include <circle/bcm2836.h>
#include <circle/bcm2711.h>
#include <circle/synchronize.h>
#include <circle/alloc.h>
#include <circle/util.h>
#include <assert.h>

#if RASPPI == 1
#define MEM_TOTAL_END	ARM_IO_END

#define TTBR_MODE	(  ARM_TTBR_INNER_CACHEABLE		\
			 | ARM_TTBR_OUTER_NON_CACHEABLE)
#elif RASPPI <= 3
#define MEM_TOTAL_END	ARM_LOCAL_END

#define TTBR_MODE	(  ARM_TTBR_INNER_WRITE_BACK		\
			 | ARM_TTBR_OUTER_WRITE_BACK)
#else
#define MEM_TOTAL_END	ARM_GIC_END
#endif

#if RASPPI <= 3

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

#else	// RASPPI <= 3

#define LEVEL1_TABLE_ENTRIES	4

CPageTable::CPageTable (u32 nMemSize)
:	m_nMemSize (nMemSize),
	m_pTable ((u64 *) MEM_PAGE_TABLE1)
{
	memset (m_pTable, 0, PAGE_SIZE);

	for (unsigned nEntry = 0; nEntry < LEVEL1_TABLE_ENTRIES; nEntry++)	// entries a 1GB
	{
		u64 nBaseAddress =
			(u64) nEntry * ARMV7LPAE_TABLE_ENTRIES * ARMV7LPAE_LEVEL2_BLOCK_SIZE;

		u64 *pTable = CreateLevel2Table (nBaseAddress);
		assert (pTable != 0);

		TARMV7LPAE_LEVEL1_TABLE_DESCRIPTOR *pDesc =
			(TARMV7LPAE_LEVEL1_TABLE_DESCRIPTOR *) &m_pTable[nEntry];

		pDesc->Value11	    = 3;
		pDesc->Ignored1	    = 0;
		pDesc->TableAddress = ARMV7LPAEL1TABLEADDR ((u64) pTable);
		pDesc->Unknown      = 0;
		pDesc->Ignored2	    = 0;
		pDesc->PXNTable	    = 0;
		pDesc->XNTable	    = 0;
		pDesc->APTable	    = AP_TABLE_ALL_ACCESS;
		pDesc->NSTable	    = 0;
	}

	DataSyncBarrier ();
}

CPageTable::~CPageTable (void)
{
}

u64 CPageTable::GetBaseAddress (void) const
{
	return (u64) m_pTable;
}

u64 *CPageTable::CreateLevel2Table (u64 nBaseAddress)
{
	u64 *pTable = (u64 *) palloc ();
	assert (pTable != 0);

	for (unsigned nEntry = 0; nEntry < ARMV7LPAE_TABLE_ENTRIES; nEntry++)	// 512 entries a 2MB
	{
		TARMV7LPAE_LEVEL2_BLOCK_DESCRIPTOR *pDesc =
			(TARMV7LPAE_LEVEL2_BLOCK_DESCRIPTOR *) &pTable[nEntry];

		pDesc->Value01	     = 1;
		pDesc->AttrIndx	     = ATTRINDX_NORMAL;
		pDesc->NS	     = 0;
		pDesc->AP	     = ATTRIB_AP_RW_PL1;
		pDesc->SH	     = ATTRIB_SH_INNER_SHAREABLE;
		pDesc->AF	     = 1;
		pDesc->nG	     = 0;
		pDesc->Unknown1      = 0;
		pDesc->OutputAddress = ARMV7LPAEL2BLOCKADDR (nBaseAddress);
		pDesc->Unknown2      = 0;
		pDesc->Continous     = 0;
		pDesc->PXN	     = 0;
		pDesc->XN	     = 0;
		pDesc->Ignored	     = 0;

		extern u8 _etext;
		if (nBaseAddress >= (u64) &_etext)
		{
			pDesc->PXN = 1;

			if (   (   nBaseAddress >= m_nMemSize
			        && nBaseAddress < MEM_HIGHMEM_START)
			    || nBaseAddress > MEM_HIGHMEM_END)
			{
				pDesc->AttrIndx = ATTRINDX_DEVICE;
				pDesc->SH	= ATTRIB_SH_OUTER_SHAREABLE;
			}
			else if (   nBaseAddress >= MEM_COHERENT_REGION
				 && nBaseAddress <  MEM_HEAP_START)
			{
				pDesc->AttrIndx = ATTRINDX_COHERENT;
				pDesc->SH	= ATTRIB_SH_OUTER_SHAREABLE;
			}

			if (nBaseAddress == MEM_PCIE_RANGE_START_VIRTUAL)
			{
				pDesc->OutputAddress = ARMV7LPAEL2BLOCKADDR (MEM_PCIE_RANGE_START);
			}
		}

		nBaseAddress += ARMV7LPAE_LEVEL2_BLOCK_SIZE;
	}

	return pTable;
}

#endif	// RASPPI <= 3
