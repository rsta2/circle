//
// memory64.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/memory.h>
#include <circle/bcmpropertytags.h>
#include <circle/machineinfo.h>
#include <circle/alloc.h>
#include <circle/spinlock.h>
#include <circle/synchronize.h>
#include <circle/sysconfig.h>
#include <assert.h>

CMemorySystem *CMemorySystem::s_pThis = 0;

CMemorySystem::CMemorySystem (boolean bEnableMMU)
:	m_bEnableMMU (bEnableMMU),
	m_nMemSize (0),
	m_nMemSizeHigh (0),
	m_HeapLow ("heaplow"),
#if RASPPI >= 4
	m_HeapHigh ("heaphigh"),
#endif
	m_pTranslationTable (0)
{
	if (s_pThis != 0)	// ignore second instance
	{
		return;
	}
	s_pThis = this;

	CBcmPropertyTags Tags (TRUE);
	TPropertyTagMemory TagMemory;
	if (!Tags.GetTag (PROPTAG_GET_ARM_MEMORY, &TagMemory, sizeof TagMemory))
	{
		TagMemory.nBaseAddress = 0;
		TagMemory.nSize = ARM_MEM_SIZE;
	}

	assert (TagMemory.nBaseAddress == 0);
	m_nMemSize = TagMemory.nSize;

	size_t nBlockReserve = m_nMemSize - MEM_HEAP_START - PAGE_RESERVE;
	m_HeapLow.Setup (MEM_HEAP_START, nBlockReserve, 0x40000);

#if RASPPI >= 4
	unsigned nRAMSize = CMachineInfo::Get ()->GetRAMSize ();
	if (nRAMSize > 1024)
	{
		u64 nHighSize = (nRAMSize - 1024) * MEGABYTE;
		if (nHighSize > MEM_HIGHMEM_END+1 - MEM_HIGHMEM_START)
		{
			nHighSize = MEM_HIGHMEM_END+1 - MEM_HIGHMEM_START;
		}

		m_nMemSizeHigh = (size_t) nHighSize;

		m_HeapHigh.Setup (MEM_HIGHMEM_START, (size_t) nHighSize, 0);
	}
#endif

	m_Pager.Setup (MEM_HEAP_START + nBlockReserve, PAGE_RESERVE);

	if (m_bEnableMMU)
	{
		m_pTranslationTable = new CTranslationTable (m_nMemSize);
		assert (m_pTranslationTable != 0);

		EnableMMU ();

#ifdef ARM_ALLOW_MULTI_CORE
		CSpinLock::Enable ();
#endif
	}
}

CMemorySystem::~CMemorySystem (void)
{
	Destructor ();
}

void CMemorySystem::Destructor (void)
{
	if (s_pThis != this)
	{
		return;
	}
	s_pThis = 0;

	if (m_bEnableMMU)
	{
		// disable MMU and data cache
		u64 nSCTLR_EL1;
		asm volatile ("mrs %0, sctlr_el1" : "=r" (nSCTLR_EL1));
		nSCTLR_EL1 &= ~(SCTLR_EL1_M | SCTLR_EL1_C);
		asm volatile ("msr sctlr_el1, %0" : : "r" (nSCTLR_EL1) : "memory");
		DataSyncBarrier ();
		InstructionSyncBarrier ();

		CleanDataCache ();
		InvalidateDataCache ();

		// invalidate TLB (if MMU is re-enabled later)
		asm volatile ("tlbi vmalle1" : : : "memory");
		DataSyncBarrier ();
		InstructionSyncBarrier ();
	}
}

#ifdef ARM_ALLOW_MULTI_CORE

void CMemorySystem::InitializeSecondary (void)
{
	assert (s_pThis != 0);
	assert (s_pThis->m_bEnableMMU);		// required to use spin locks

	s_pThis->EnableMMU ();
}

#endif

size_t CMemorySystem::GetMemSize (void) const
{
	assert (s_pThis != 0);
	return s_pThis->m_nMemSize + s_pThis->m_nMemSizeHigh;
}

CMemorySystem *CMemorySystem::Get (void)
{
	assert (s_pThis != 0);
	return s_pThis;
}

void CMemorySystem::EnableMMU (void)
{
	assert (m_bEnableMMU);

	u64 nMAIR_EL1 =   0xFF << ATTRINDX_NORMAL*8	// inner/outer write-back non-transient, allocating
	                | 0x04 << ATTRINDX_DEVICE*8	// Device-nGnRE
	                | 0x00 << ATTRINDX_COHERENT*8;	// Device-nGnRnE
	asm volatile ("msr mair_el1, %0" : : "r" (nMAIR_EL1));

	assert (m_pTranslationTable != 0);
	asm volatile ("msr ttbr0_el1, %0" : : "r" (m_pTranslationTable->GetBaseAddress ()));

	u64 nTCR_EL1;
	asm volatile ("mrs %0, tcr_el1" : "=r" (nTCR_EL1));
	nTCR_EL1 &= ~(  TCR_EL1_IPS__MASK
		      | TCR_EL1_A1
		      | TCR_EL1_TG0__MASK
		      | TCR_EL1_SH0__MASK
		      | TCR_EL1_ORGN0__MASK
		      | TCR_EL1_IRGN0__MASK
		      | TCR_EL1_EPD0
		      | TCR_EL1_T0SZ__MASK);
	nTCR_EL1 |=   TCR_EL1_EPD1
		    | TCR_EL1_TG0_64KB	    << TCR_EL1_TG0__SHIFT
		    | TCR_EL1_SH0_INNER     << TCR_EL1_SH0__SHIFT
		    | TCR_EL1_ORGN0_WR_BACK_ALLOCATE << TCR_EL1_ORGN0__SHIFT
		    | TCR_EL1_IRGN0_WR_BACK_ALLOCATE << TCR_EL1_IRGN0__SHIFT
#if RASPPI == 3
		    | TCR_EL1_IPS_4GB	    << TCR_EL1_IPS__SHIFT
		    | TCR_EL1_T0SZ_4GB	    << TCR_EL1_T0SZ__SHIFT;
#else
		    | TCR_EL1_IPS_64GB	    << TCR_EL1_IPS__SHIFT
		    | TCR_EL1_T0SZ_64GB	    << TCR_EL1_T0SZ__SHIFT;
#endif
	asm volatile ("msr tcr_el1, %0" : : "r" (nTCR_EL1));

	u64 nSCTLR_EL1;
	asm volatile ("mrs %0, sctlr_el1" : "=r" (nSCTLR_EL1));
	nSCTLR_EL1 &= ~(  SCTLR_EL1_WXN
			| SCTLR_EL1_A);
	nSCTLR_EL1 |=   SCTLR_EL1_I
		      | SCTLR_EL1_C
		      | SCTLR_EL1_M;
	asm volatile ("msr sctlr_el1, %0" : : "r" (nSCTLR_EL1) : "memory");
}

uintptr CMemorySystem::GetCoherentPage (unsigned nSlot)
{
	u64 nPageAddress = MEM_COHERENT_REGION;

	nPageAddress += nSlot * PAGE_SIZE;

	return nPageAddress;
}
