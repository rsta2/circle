//
// heapallocator.h
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
#ifndef _circle_heapallocator_h
#define _circle_heapallocator_h

#include <circle/spinlock.h>
#include <circle/synchronize.h>
#include <circle/sysconfig.h>
#include <circle/macros.h>
#include <circle/types.h>
#include <assert.h>

//#define HEAP_DEBUG

ASSERT_STATIC (DATA_CACHE_LINE_LENGTH_MAX >= 16);

#define HEAP_BLOCK_ALIGN	DATA_CACHE_LINE_LENGTH_MAX
#define HEAP_ALIGN_MASK		(HEAP_BLOCK_ALIGN-1)

#define HEAP_BLOCK_MAX_BUCKETS	20

struct THeapBlockHeader
{
	u32			 nMagic;
#define HEAP_BLOCK_MAGIC	0x424C4D43
	u32			 nSize;
	THeapBlockHeader	*pNext;
#if AARCH == 32
	u32			 nPadding;
#endif
	u8			 Align[HEAP_BLOCK_ALIGN-16];
	u8			 Data[0];
}
PACKED;

struct THeapBlockBucket
{
	u32			 nSize;
#ifdef HEAP_DEBUG
	unsigned		 nCount;
	unsigned		 nMaxCount;
#endif
	THeapBlockHeader	*pFreeList;
};

class CHeapAllocator	/// Allocates blocks from a flat memory region
{
public:
	/// \param pHeapName Name of the heap for debugging purpose (must be static)
	CHeapAllocator (const char *pHeapName = "heap");

	~CHeapAllocator (void);

	/// \param nBase    Base address of memory region (must be 16 bytes aligned)
	/// \param nSize    Size of memory region
	/// \param nReserve Free space reserved for handling of "Out of memory" messages\n
	///		    (Allocate() returns 0, if nReserve is 0 and memory region is full)
	void Setup (uintptr nBase, size_t nSize, size_t nReserve);

	/// \return Free space of the memory region, which is not allocated by blocks
	/// \note Unused blocks on a free list do not count here.
	size_t GetFreeSpace (void) const;

	/// \param nSize Block size to be allocated
	/// \return Pointer to new allocated block (0 if heap is full or not set-up)
	/// \note Resulting block is always 16 bytes aligned
	/// \note If nReserve in Setup() is non-zero, the system panics if heap is full.
	void *Allocate (size_t nSize);

	/// \param pBlock Memory block to be reallocated
	/// \param nSize  New block size
	/// \return Pointer to new block (block contents has been copied, if the block has moved)
	void *ReAllocate (void *pBlock, size_t nSize);

	/// \param pBlock Memory block to be freed
	/// \note Memory space of blocks, which are bigger than the largest bucket size,\n
	///	  cannot be returned to a free list and is lost.
	void Free (void *pBlock);

#ifdef HEAP_DEBUG
	void DumpStatus (void);
#endif

private:
	const char	*m_pHeapName;
	u8		*m_pNext;
	u8		*m_pLimit;
	size_t	 	 m_nReserve;
	THeapBlockBucket m_Bucket[HEAP_BLOCK_MAX_BUCKETS+1];
	CSpinLock	 m_SpinLock;

	static u32 s_nBucketSize[];
};

#endif
