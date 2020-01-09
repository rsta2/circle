//
// translationtable64.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2020  R. Stange <rsta2@o2online.de>
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
#include <circle/translationtable64.h>
#include <circle/synchronize.h>
#include <circle/sysconfig.h>
#include <circle/alloc.h>
#include <circle/util.h>
#include <assert.h>

// Granule size is 64KB. Only EL1 stage 1 translation is enabled with 32 bits IPA
// (= PA) size (4GB).

#if RASPPI == 3
// We create one level 2 (first lookup level) translation table with 3 table
// entries (total 1.5GB) which point to a level 3 (final lookup level) translation
// table each with 8192 page entries a 64KB (total 512MB).
#define LEVEL2_TABLE_ENTRIES	3
#elif RASPPI >= 4
// We create one level 2 (first lookup level) translation table with 128 table
// entries (total 64GB) which point to a level 3 (final lookup level) translation
// table each with 8192 page entries a 64KB (total 512MB).
#define LEVEL2_TABLE_ENTRIES	128
#endif

CTranslationTable::CTranslationTable (size_t nMemSize)
:	m_nMemSize (nMemSize),
	m_pTable (0)
{
	m_pTable = (TARMV8MMU_LEVEL2_DESCRIPTOR *) palloc ();
	assert (m_pTable != 0);

	memset (m_pTable, 0, PAGE_SIZE);

	for (unsigned nEntry = 0; nEntry < LEVEL2_TABLE_ENTRIES; nEntry++)	// entries a 512MB
	{
		u64 nBaseAddress = (u64) nEntry * ARMV8MMU_TABLE_ENTRIES * ARMV8MMU_LEVEL3_PAGE_SIZE;

#if RASPPI >= 4
		if (   nBaseAddress >= 4*GIGABYTE
		    && !(   MEM_PCIE_RANGE_START_VIRTUAL <= nBaseAddress
			 && nBaseAddress <= MEM_PCIE_RANGE_END_VIRTUAL))
		{
			continue;
		}
#endif

		TARMV8MMU_LEVEL3_DESCRIPTOR *pTable = CreateLevel3Table (nBaseAddress);
		assert (pTable != 0);

		TARMV8MMU_LEVEL2_TABLE_DESCRIPTOR *pDesc = &m_pTable[nEntry].Table;

		pDesc->Value11	    = 3;
		pDesc->Ignored1	    = 0;
		pDesc->TableAddress = ARMV8MMUL2TABLEADDR ((u64) pTable);
		pDesc->Reserved0    = 0;
		pDesc->Ignored2	    = 0;
		pDesc->PXNTable	    = 0;
		pDesc->UXNTable	    = 0;
		pDesc->APTable	    = AP_TABLE_ALL_ACCESS;
		pDesc->NSTable	    = 0;
	}

	DataSyncBarrier ();
}

CTranslationTable::~CTranslationTable (void)
{
	pfree (m_pTable);
	m_pTable = 0;
}

uintptr CTranslationTable::GetBaseAddress (void) const
{
	assert (m_pTable != 0);
	return (uintptr) m_pTable;
}

TARMV8MMU_LEVEL3_DESCRIPTOR *CTranslationTable::CreateLevel3Table (uintptr nBaseAddress)
{
	TARMV8MMU_LEVEL3_DESCRIPTOR *pTable = (TARMV8MMU_LEVEL3_DESCRIPTOR *) palloc ();
	assert (pTable != 0);

	for (unsigned nPage = 0; nPage < ARMV8MMU_TABLE_ENTRIES; nPage++)	// 8192 entries a 64KB
	{
		TARMV8MMU_LEVEL3_PAGE_DESCRIPTOR *pDesc = &pTable[nPage].Page;

		pDesc->Value11	     = 3;
		pDesc->AttrIndx	     = ATTRINDX_NORMAL;
		pDesc->NS	     = 0;
		pDesc->AP	     = ATTRIB_AP_RW_EL1;
		pDesc->SH	     = ATTRIB_SH_INNER_SHAREABLE;
		pDesc->AF	     = 1;
		pDesc->nG	     = 0;
		pDesc->Reserved0_1   = 0;
		pDesc->OutputAddress = ARMV8MMUL3PAGEADDR (nBaseAddress);
		pDesc->Reserved0_2   = 0;
		pDesc->Continous     = 0;
		pDesc->PXN	     = 0;
		pDesc->UXN	     = 1;
		pDesc->Ignored	     = 0;

		extern u8 _etext;
		if (nBaseAddress >= (u64) &_etext)
		{
			pDesc->PXN = 1;

#if RASPPI >= 4
			if (   (   nBaseAddress >= m_nMemSize
			        && nBaseAddress < MEM_HIGHMEM_START)
			    || nBaseAddress > MEM_HIGHMEM_END)
#else
			if (nBaseAddress >= m_nMemSize)
#endif
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
		}

		nBaseAddress += ARMV8MMU_LEVEL3_PAGE_SIZE;
	}

	return pTable;
}
