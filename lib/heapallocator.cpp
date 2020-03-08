//
// heapallocator.cpp
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
#include <circle/heapallocator.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <assert.h>

u32 CHeapAllocator::s_nBucketSize[] = { HEAP_BLOCK_BUCKET_SIZES };

CHeapAllocator::CHeapAllocator (const char *pHeapName)
:	m_pHeapName (pHeapName),
	m_pNext (0),
	m_pLimit (0),
	m_nReserve (0)
{
	memset (m_Bucket, 0, sizeof m_Bucket);

	unsigned nBuckets = sizeof s_nBucketSize / sizeof s_nBucketSize[0];
	if (nBuckets > HEAP_BLOCK_MAX_BUCKETS)
	{
		nBuckets = HEAP_BLOCK_MAX_BUCKETS;
	}

	for (unsigned i = 0; i < nBuckets; i++)
	{
		m_Bucket[i].nSize = s_nBucketSize[i];
	}
}

CHeapAllocator::~CHeapAllocator (void)
{
}

void CHeapAllocator::Setup (uintptr nBase, size_t nSize, size_t nReserve)
{
	m_pNext = (u8 *) nBase;
	m_pLimit = (u8 *) (nBase + nSize);
	m_nReserve = nReserve;
}

size_t CHeapAllocator::GetFreeSpace (void) const
{
	return m_pLimit - m_pNext;
}

void *CHeapAllocator::Allocate (size_t nSize)
{
	if (m_pNext == 0)
	{
		return 0;
	}

	m_SpinLock.Acquire ();

	THeapBlockBucket *pBucket;
	for (pBucket = m_Bucket; pBucket->nSize > 0; pBucket++)
	{
		if (nSize <= pBucket->nSize)
		{
			nSize = pBucket->nSize;

#ifdef HEAP_DEBUG
			if (++pBucket->nCount > pBucket->nMaxCount)
			{
				pBucket->nMaxCount = pBucket->nCount;
			}
#endif

			break;
		}
	}

	THeapBlockHeader *pBlockHeader;
	if (   pBucket->nSize > 0
	    && (pBlockHeader = pBucket->pFreeList) != 0)
	{
		assert (pBlockHeader->nMagic == HEAP_BLOCK_MAGIC);
		pBucket->pFreeList = pBlockHeader->pNext;
	}
	else
	{
		pBlockHeader = (THeapBlockHeader *) m_pNext;

		u8 *pNextBlock = m_pNext;
		pNextBlock += (sizeof (THeapBlockHeader) + nSize + HEAP_BLOCK_ALIGN-1) & ~HEAP_ALIGN_MASK;

		if (   pNextBlock <= m_pNext			// may have wrapped
		    || pNextBlock > m_pLimit-m_nReserve)
		{
			if (m_nReserve == 0)
			{
				m_SpinLock.Release ();

				return 0;
			}

			m_nReserve = 0;

			m_SpinLock.Release ();

#ifdef HEAP_DEBUG
			DumpStatus ();
#endif
#if STDLIB_SUPPORT == 3
			// C++ exception should be thrown after returning 0
			CLogger::Get ()->WriteNoAlloc (m_pHeapName, LogWarning, "Out of memory");
#else
			CLogger::Get ()->Write (m_pHeapName, LogPanic, "Out of memory");
#endif

			return 0;
		}

		m_pNext = pNextBlock;

		pBlockHeader->nMagic = HEAP_BLOCK_MAGIC;
		pBlockHeader->nSize = (u32) nSize;
	}

	m_SpinLock.Release ();

	pBlockHeader->pNext = 0;

	void *pResult = pBlockHeader->Data;
	assert (((uintptr) pResult & HEAP_ALIGN_MASK) == 0);

	return pResult;
}

void *CHeapAllocator::ReAllocate (void *pBlock, size_t nSize)
{
	if (pBlock == 0)
	{
		return Allocate (nSize);
	}

	if (nSize == 0)
	{
		Free (pBlock);

		return 0;
	}

	THeapBlockHeader *pBlockHeader =
		(THeapBlockHeader *) ((uintptr) pBlock - sizeof (THeapBlockHeader));
	assert (pBlockHeader->nMagic == HEAP_BLOCK_MAGIC);
	if (pBlockHeader->nSize >= nSize)
	{
		return pBlock;
	}

	void *pNewBlock = Allocate (nSize);
	if (pNewBlock == 0)
	{
		return 0;
	}

	memcpy (pNewBlock, pBlock, pBlockHeader->nSize);

	Free (pBlock);

	return pNewBlock;
}

void CHeapAllocator::Free (void *pBlock)
{
	if (pBlock == 0)
	{
		return;
	}

	THeapBlockHeader *pBlockHeader =
		(THeapBlockHeader *) ((uintptr) pBlock - sizeof (THeapBlockHeader));
	assert (pBlockHeader->nMagic == HEAP_BLOCK_MAGIC);

	for (THeapBlockBucket *pBucket = m_Bucket; pBucket->nSize > 0; pBucket++)
	{
		if (pBlockHeader->nSize == pBucket->nSize)
		{
			m_SpinLock.Acquire ();

			pBlockHeader->pNext = pBucket->pFreeList;
			pBucket->pFreeList = pBlockHeader;

#ifdef HEAP_DEBUG
			pBucket->nCount--;
#endif

			m_SpinLock.Release ();

			return;
		}
	}

#ifdef HEAP_DEBUG
	CLogger::Get ()->Write (m_pHeapName, LogDebug, "Trying to free large block (size %u)",
				pBlockHeader->nSize);
#endif
}

#ifdef HEAP_DEBUG

void CHeapAllocator::DumpStatus (void)
{
	for (THeapBlockBucket *pBucket = m_Bucket; pBucket->nSize > 0; pBucket++)
	{
		CLogger::Get ()->Write (m_pHeapName, LogDebug, "malloc(%lu): %u blocks (max %u)",
					pBucket->nSize, pBucket->nCount, pBucket->nMaxCount);
	}
}

#endif
