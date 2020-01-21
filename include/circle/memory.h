//
// memory.h
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
#ifndef _circle_memory_h
#define _circle_memory_h

#if AARCH == 32
	#include <circle/pagetable.h>
#else
	#include <circle/translationtable64.h>
#endif

#include <circle/heapallocator.h>
#include <circle/pageallocator.h>
#include <circle/sysconfig.h>
#include <circle/types.h>

class CMemorySystem
{
public:
	CMemorySystem (boolean bEnableMMU = TRUE);
	~CMemorySystem (void);

	void Destructor (void);		// explicit callable

#ifdef ARM_ALLOW_MULTI_CORE
	void InitializeSecondary (void);
#endif

	size_t GetMemSize (void) const;

	static uintptr GetCoherentPage (unsigned nSlot);
#define COHERENT_SLOT_PROP_MAILBOX	0
#define COHERENT_SLOT_GPIO_VIRTBUF	1
#define COHERENT_SLOT_TOUCHBUF		2

#define COHERENT_SLOT_VCHIQ_START	(MEGABYTE / PAGE_SIZE / 2)
#define COHERENT_SLOT_VCHIQ_END		(MEGABYTE / PAGE_SIZE - 1)

#if RASPPI >= 4
#define COHERENT_SLOT_XHCI_START	(MEGABYTE / PAGE_SIZE)
#define COHERENT_SLOT_XHCI_END		(4*MEGABYTE / PAGE_SIZE - 1)
#endif

	static CMemorySystem *Get (void);

public:
	static void *HeapAllocate (size_t nSize, int nType)
#define HEAP_LOW	0		// memory below 1 GB
#define HEAP_HIGH	1		// memory above 1 GB
#define HEAP_ANY	2		// high memory (if available) or low memory (otherwise)
#define HEAP_DMA30	HEAP_LOW	// 30-bit DMA-able memory
	{
#if RASPPI >= 4
		void *pBlock;

		switch (nType)
		{
		case HEAP_LOW:	return s_pThis->m_HeapLow.Allocate (nSize);
		case HEAP_HIGH: return s_pThis->m_HeapHigh.Allocate (nSize);
		case HEAP_ANY:	return   (pBlock = s_pThis->m_HeapHigh.Allocate (nSize)) != 0
				       ? pBlock
				       : s_pThis->m_HeapLow.Allocate (nSize);
		default:	return 0;
		}
#else
		switch (nType)
		{
		case HEAP_LOW:
		case HEAP_ANY:	return s_pThis->m_HeapLow.Allocate (nSize);
		default:	return 0;
		}
#endif
	}

	static void *HeapReAllocate (void *pBlock, size_t nSize)	// pBlock may be 0
	{
#if RASPPI >= 4
		if ((uintptr) pBlock < MEM_HIGHMEM_START)
		{
			return s_pThis->m_HeapLow.ReAllocate (pBlock, nSize);
		}
		else
		{
			return s_pThis->m_HeapHigh.ReAllocate (pBlock, nSize);
		}
#else
		return s_pThis->m_HeapLow.ReAllocate (pBlock, nSize);
#endif
	}

	static void HeapFree (void *pBlock)
	{
#if RASPPI >= 4
		if ((uintptr) pBlock < MEM_HIGHMEM_START)
		{
			s_pThis->m_HeapLow.Free (pBlock);
		}
		else
		{
			s_pThis->m_HeapHigh.Free (pBlock);
		}
#else
		s_pThis->m_HeapLow.Free (pBlock);
#endif
	}

	size_t GetHeapFreeSpace (int nType) const
	{
#if RASPPI >= 4
		switch (nType)
		{
		case HEAP_LOW:	return s_pThis->m_HeapLow.GetFreeSpace ();
		case HEAP_HIGH: return s_pThis->m_HeapHigh.GetFreeSpace ();
		case HEAP_ANY:	return   s_pThis->m_HeapLow.GetFreeSpace ()
				       + s_pThis->m_HeapHigh.GetFreeSpace ();
		default:	return 0;
		}
#else
		switch (nType)
		{
		case HEAP_LOW:
		case HEAP_ANY:	return s_pThis->m_HeapLow.GetFreeSpace ();
		default:	return 0;
		}
#endif
	}

	static void *PageAllocate (void)	{ return s_pThis->m_Pager.Allocate (); }
	static void PageFree (void *pPage)	{ s_pThis->m_Pager.Free (pPage); }

	static void DumpStatus (void)
	{
#ifdef HEAP_DEBUG
		s_pThis->m_HeapLow.DumpStatus ();
#if RASPPI >= 4
		s_pThis->m_HeapHigh.DumpStatus ();
#endif
#endif

#ifdef PAGE_DEBUG
		s_pThis->m_Pager.DumpStatus ();
#endif
	}

private:
	void EnableMMU (void);

private:
	boolean m_bEnableMMU;
	size_t m_nMemSize;
	size_t m_nMemSizeHigh;

	CHeapAllocator m_HeapLow;
#if RASPPI >= 4
	CHeapAllocator m_HeapHigh;
#endif
	CPageAllocator m_Pager;

#if AARCH == 32
	CPageTable *m_pPageTable;
#else
	CTranslationTable *m_pTranslationTable;
#endif

	static CMemorySystem *s_pThis;
};

#endif
