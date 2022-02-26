//
// memory.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2022  R. Stange <rsta2@o2online.de>
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
#include <circle/armv6mmu.h>
#include <circle/armv7lpae.h>
#include <circle/bcmpropertytags.h>
#include <circle/machineinfo.h>
#include <circle/alloc.h>
#include <circle/synchronize.h>
#include <circle/spinlock.h>
#include <circle/sysconfig.h>
#include <assert.h>

#if RASPPI == 1
#define MMU_MODE	(  ARM_CONTROL_MMU			\
			 | ARM_CONTROL_L1_CACHE			\
			 | ARM_CONTROL_L1_INSTRUCTION_CACHE	\
			 | ARM_CONTROL_BRANCH_PREDICTION	\
			 | ARM_CONTROL_EXTENDED_PAGE_TABLE)
#else
#define MMU_MODE	(  ARM_CONTROL_MMU			\
			 | ARM_CONTROL_L1_CACHE			\
			 | ARM_CONTROL_L1_INSTRUCTION_CACHE	\
			 | ARM_CONTROL_BRANCH_PREDICTION)
#endif

#define TTBCR_SPLIT	0

CMemorySystem *CMemorySystem::s_pThis = 0;

CMemorySystem::CMemorySystem (boolean bEnableMMU)
#ifndef USE_RPI_STUB_AT
:	m_bEnableMMU (bEnableMMU),
	m_nMemSize (0),
#else
:	m_bEnableMMU (FALSE),
	m_nMemSize (USE_RPI_STUB_AT),
#endif
	m_nMemSizeHigh (0),
	m_HeapLow ("heaplow"),
#if RASPPI >= 4
	m_HeapHigh ("heaphigh"),
#endif
	m_pPageTable (0)
{
	if (s_pThis != 0)	// ignore second instance
	{
		return;
	}
	s_pThis = this;

#ifndef USE_RPI_STUB_AT
	CBcmPropertyTags Tags (TRUE);
	TPropertyTagMemory TagMemory;
	if (!Tags.GetTag (PROPTAG_GET_ARM_MEMORY, &TagMemory, sizeof TagMemory))
	{
		TagMemory.nBaseAddress = 0;
		TagMemory.nSize = ARM_MEM_SIZE;
	}

	assert (TagMemory.nBaseAddress == 0);
	m_nMemSize = TagMemory.nSize;
#endif

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
		m_pPageTable = new CPageTable (m_nMemSize);
		assert (m_pPageTable != 0);

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
		// disable MMU
		u32 nControl;
		asm volatile ("mrc p15, 0, %0, c1, c0,  0" : "=r" (nControl));
		nControl &=  ~(ARM_CONTROL_MMU | ARM_CONTROL_L1_CACHE);
		asm volatile ("mcr p15, 0, %0, c1, c0,  0" : : "r" (nControl) : "memory");

		CleanDataCache ();
		InvalidateDataCache ();

		// invalidate unified TLB (if MMU is re-enabled later)
		asm volatile ("mcr p15, 0, %0, c8, c7,  0" : : "r" (0) : "memory");
		DataSyncBarrier ();
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

#if RASPPI <= 3
	u32 nAuxControl;
	asm volatile ("mrc p15, 0, %0, c1, c0,  1" : "=r" (nAuxControl));
#if RASPPI == 1
	nAuxControl |= ARM_AUX_CONTROL_CACHE_SIZE;	// restrict cache size (no page coloring)
#else
	nAuxControl |= ARM_AUX_CONTROL_SMP;
#endif
	asm volatile ("mcr p15, 0, %0, c1, c0,  1" : : "r" (nAuxControl));

	u32 nTLBType;
	asm volatile ("mrc p15, 0, %0, c0, c0,  3" : "=r" (nTLBType));
	assert (!(nTLBType & ARM_TLB_TYPE_SEPARATE_TLBS));

	// set TTB control
	asm volatile ("mcr p15, 0, %0, c2, c0,  2" : : "r" (TTBCR_SPLIT));

	// set TTBR0
	assert (m_pPageTable != 0);
	asm volatile ("mcr p15, 0, %0, c2, c0,  0" : : "r" (m_pPageTable->GetBaseAddress ()));
#else	// RASPPI <= 3
	// set MAIR0
	u32 nMAIR0 =   LPAE_MAIR_NORMAL   << ATTRINDX_NORMAL*8
                     | LPAE_MAIR_DEVICE   << ATTRINDX_DEVICE*8
	             | LPAE_MAIR_COHERENT << ATTRINDX_COHERENT*8;
	asm volatile ("mcr p15, 0, %0, c10, c2, 0" : : "r" (nMAIR0));

	// set TTBCR
	asm volatile ("mcr p15, 0, %0, c2, c0,  2" : : "r" (
		        LPAE_TTBCR_EAE
		      | LPAE_TTBCR_EPD1
		      | ATTRIB_SH_INNER_SHAREABLE << LPAE_TTBCR_SH0__SHIFT
		      | LPAE_TTBCR_ORGN0_WR_BACK_ALLOCATE << LPAE_TTBCR_ORGN0__SHIFT
		      | LPAE_TTBCR_IRGN0_WR_BACK_ALLOCATE << LPAE_TTBCR_IRGN0__SHIFT
		      | LPAE_TTBCR_T0SZ_4GB));

	// set TTBR0
	assert (m_pPageTable != 0);
	u64 nBaseAddress = m_pPageTable->GetBaseAddress ();
	asm volatile ("mcrr p15, 0, %0, %1, c2" : : "r" ((u32) nBaseAddress),
						    "r" ((u32) (nBaseAddress >> 32)));
#endif	// RASPPI <= 3

	// set Domain Access Control register (Domain 0 to client)
	asm volatile ("mcr p15, 0, %0, c3, c0,  0" : : "r" (DOMAIN_CLIENT << 0));

#ifndef ARM_ALLOW_MULTI_CORE
	InvalidateDataCache ();
#else
	InvalidateDataCacheL1Only ();
#endif

	// required if MMU was previously enabled and not properly reset
	InvalidateInstructionCache ();
	FlushBranchTargetCache ();
	asm volatile ("mcr p15, 0, %0, c8, c7,  0" : : "r" (0));	// invalidate unified TLB
	DataSyncBarrier ();
	FlushPrefetchBuffer ();

	// enable MMU
	u32 nControl;
	asm volatile ("mrc p15, 0, %0, c1, c0,  0" : "=r" (nControl));
#if RASPPI == 1
#ifdef ARM_STRICT_ALIGNMENT
	nControl &= ~ARM_CONTROL_UNALIGNED_PERMITTED;
	nControl |= ARM_CONTROL_STRICT_ALIGNMENT;
#else
	nControl &= ~ARM_CONTROL_STRICT_ALIGNMENT;
	nControl |= ARM_CONTROL_UNALIGNED_PERMITTED;
#endif
#else
	nControl &= ~ARM_CONTROL_STRICT_ALIGNMENT;
#endif
	nControl |= MMU_MODE;
	asm volatile ("mcr p15, 0, %0, c1, c0,  0" : : "r" (nControl) : "memory");
}

u32 CMemorySystem::GetCoherentPage (unsigned nSlot)
{
#ifndef USE_RPI_STUB_AT
	u32 nPageAddress = MEM_COHERENT_REGION;
#else
	u32 nPageAddress;
	u32 nSize;

	asm volatile
	(
		"push {r0-r1}\n"
		"mov r0, #1\n"
		"bkpt #0x7FFA\n"	// get coherent region from rpi_stub
		"mov %0, r0\n"
		"mov %1, r1\n"
		"pop {r0-r1}\n"

		: "=r" (nPageAddress), "=r" (nSize)
	);
#endif

	nPageAddress += nSlot * PAGE_SIZE;

	return nPageAddress;
}
