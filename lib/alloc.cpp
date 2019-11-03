//
// alloc.cpp
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
#include <circle/alloc.h>
#include <circle/spinlock.h>
#include <circle/sysconfig.h>
#include <circle/util.h>
#include <circle/logger.h>
#include <circle/macros.h>
#include <assert.h>

#define BLOCK_ALIGN	16
#define ALIGN_MASK	(BLOCK_ALIGN-1)

#define PAGE_MASK	(PAGE_SIZE-1)

struct TBlockHeader
{
	unsigned int	nMagic		PACKED;
#define BLOCK_MAGIC	0x424C4D43
	unsigned int	nSize		PACKED;
	TBlockHeader	*pNext		PACKED;
#if AARCH == 32
	unsigned int	nPadding	PACKED;
#endif
	unsigned char	Data[0];
}
PACKED;

struct TBlockBucket
{
	unsigned int	nSize;
#ifdef MEM_DEBUG
	unsigned int	nCount;
	unsigned int	nMaxCount;
#endif
	TBlockHeader	*pFreeList;
};

struct TFreePage
{
	unsigned int	nMagic;
#define FREEPAGE_MAGIC	0x50474D43
	TFreePage	*pNext;
};

struct TPageBucket
{
#ifdef MEM_DEBUG
	unsigned int	nCount;
	unsigned int	nMaxCount;
#endif
	TFreePage	*pFreeList;
};

static unsigned char *s_pNextBlock;
static unsigned char *s_pBlockLimit;
static unsigned       s_nBlockReserve = 0x40000;

static unsigned char *s_pNextPage;
static unsigned char *s_pPageLimit;

static TBlockBucket s_BlockBucket[] = {{0x40}, {0x400}, {0x1000}, {0x4000}, {0x10000}, {0x40000}, {0x80000}, {0}};

static TPageBucket s_PageBucket;

static CSpinLock s_BlockSpinLock;
static CSpinLock s_PageSpinLock;

void mem_init (unsigned long ulBase, unsigned long ulSize)
{
	unsigned long ulLimit = ulBase + ulSize;

	if (ulBase < MEM_HEAP_START)
	{
		ulBase = MEM_HEAP_START;
	}
	
	ulSize = ulLimit - ulBase;
	unsigned long ulBlockReserve = ulSize - PAGE_RESERVE;

	s_pNextBlock = (unsigned char *) ulBase;
	s_pBlockLimit = (unsigned char *) (ulBase + ulBlockReserve);

	s_pNextPage = (unsigned char *) ((ulBase + ulBlockReserve + PAGE_SIZE) & ~PAGE_MASK);
	s_pPageLimit = (unsigned char *) ulLimit;
}

unsigned long mem_get_size (void)
{
	return (unsigned long) (s_pBlockLimit - s_pNextBlock) + (s_pPageLimit - s_pNextPage);
}

void *malloc (size_t nSize)
{
	assert (s_pNextBlock != 0);
	
	s_BlockSpinLock.Acquire ();

	TBlockBucket *pBucket;
	for (pBucket = s_BlockBucket; pBucket->nSize > 0; pBucket++)
	{
		if (nSize <= pBucket->nSize)
		{
			nSize = pBucket->nSize;

#ifdef MEM_DEBUG
			if (++pBucket->nCount > pBucket->nMaxCount)
			{
				pBucket->nMaxCount = pBucket->nCount;
			}
#endif

			break;
		}
	}

	TBlockHeader *pBlockHeader;
	if (   pBucket->nSize > 0
	    && (pBlockHeader = pBucket->pFreeList) != 0)
	{
		assert (pBlockHeader->nMagic == BLOCK_MAGIC);
		pBucket->pFreeList = pBlockHeader->pNext;
	}
	else
	{
		pBlockHeader = (TBlockHeader *) s_pNextBlock;

		unsigned char *pNextBlock = s_pNextBlock;
		pNextBlock += (sizeof (TBlockHeader) + nSize + BLOCK_ALIGN-1) & ~ALIGN_MASK;

		if (   pNextBlock <= s_pNextBlock			// may have wrapped
		    || pNextBlock > s_pBlockLimit-s_nBlockReserve)
		{
			s_nBlockReserve = 0;

			s_BlockSpinLock.Release ();

#ifdef MEM_DEBUG
			mem_info ();
#endif
#if STDLIB_SUPPORT == 3
			// C++ exception should be thrown after returning 0
			CLogger::Get ()->WriteNoAlloc ("alloc", LogWarning, "Out of memory");
#else
			CLogger::Get ()->Write ("alloc", LogPanic, "Out of memory");
#endif

			return 0;
		}

		s_pNextBlock = pNextBlock;
	
		pBlockHeader->nMagic = BLOCK_MAGIC;
		pBlockHeader->nSize = (unsigned) nSize;
	}
	
	s_BlockSpinLock.Release ();

	pBlockHeader->pNext = 0;

	void *pResult = pBlockHeader->Data;
	assert (((unsigned long) pResult & ALIGN_MASK) == 0);

	return pResult;
}

void free (void *pBlock)
{
	if (pBlock == 0)
	{
		return;
	}

	TBlockHeader *pBlockHeader = (TBlockHeader *) ((unsigned long) pBlock - sizeof (TBlockHeader));
	assert (pBlockHeader->nMagic == BLOCK_MAGIC);

	for (TBlockBucket *pBucket = s_BlockBucket; pBucket->nSize > 0; pBucket++)
	{
		if (pBlockHeader->nSize == pBucket->nSize)
		{
			s_BlockSpinLock.Acquire ();

			pBlockHeader->pNext = pBucket->pFreeList;
			pBucket->pFreeList = pBlockHeader;

#ifdef MEM_DEBUG
			pBucket->nCount--;
#endif

			s_BlockSpinLock.Release ();

			return;
		}
	}

#ifdef MEM_DEBUG
	CLogger::Get ()->Write ("alloc", LogDebug, "Trying to free large block (size %u)",
				pBlockHeader->nSize);
#endif
}

void *calloc (size_t nBlocks, size_t nSize)
{
	nSize *= nBlocks;
	if (nSize == 0)
	{
		nSize = 1;
	}
	assert (nSize >= nBlocks);

	void *pNewBlock = malloc (nSize);
	if (pNewBlock != 0)
	{
		memset (pNewBlock, 0, nSize);
	}

	return pNewBlock;
}

void *realloc (void *pBlock, size_t nSize)
{
	if (pBlock == 0)
	{
		return malloc (nSize);
	}

	if (nSize == 0)
	{
		free (pBlock);

		return 0;
	}

	TBlockHeader *pBlockHeader = (TBlockHeader *) ((unsigned long) pBlock - sizeof (TBlockHeader));
	assert (pBlockHeader->nMagic == BLOCK_MAGIC);
	if (pBlockHeader->nSize >= nSize)
	{
		return pBlock;
	}

	void *pNewBlock = malloc (nSize);
	if (pNewBlock == 0)
	{
		return 0;
	}

	memcpy (pNewBlock, pBlock, pBlockHeader->nSize);

	free (pBlock);

	return pNewBlock;
}

void *palloc (void)
{
	assert (s_pNextPage != 0);

	s_PageSpinLock.Acquire ();

#ifdef MEM_DEBUG
	if (++s_PageBucket.nCount > s_PageBucket.nMaxCount)
	{
		s_PageBucket.nMaxCount = s_PageBucket.nCount;
	}
#endif

	TFreePage *pFreePage;
	if ((pFreePage = s_PageBucket.pFreeList) != 0)
	{
		assert (pFreePage->nMagic == FREEPAGE_MAGIC);
		s_PageBucket.pFreeList = pFreePage->pNext;
		pFreePage->nMagic = 0;
	}
	else
	{
		pFreePage = (TFreePage *) s_pNextPage;
		
		s_pNextPage += PAGE_SIZE;

		if (s_pNextPage > s_pPageLimit)
		{
			s_PageSpinLock.Release ();

			return 0;		// TODO: system should panic here
		}
	}

	s_PageSpinLock.Release ();
	
	return pFreePage;
}

void pfree (void *pPage)
{
	if (pPage == 0)
	{
		return;
	}

	TFreePage *pFreePage = (TFreePage *) pPage;
	
	s_PageSpinLock.Acquire ();

	pFreePage->nMagic = FREEPAGE_MAGIC;

	pFreePage->pNext = s_PageBucket.pFreeList;
	s_PageBucket.pFreeList = pFreePage;

#ifdef MEM_DEBUG
	s_PageBucket.nCount--;
#endif

	s_PageSpinLock.Release ();
}

#ifdef MEM_DEBUG

void mem_info (void)
{
	for (TBlockBucket *pBucket = s_BlockBucket; pBucket->nSize > 0; pBucket++)
	{
		CLogger::Get ()->Write ("alloc", LogDebug, "malloc(%lu): %u blocks (max %u)",
					pBucket->nSize, pBucket->nCount, pBucket->nMaxCount);
	}

	CLogger::Get ()->Write ("alloc", LogDebug, "palloc: %u pages (max %u)",
				s_PageBucket.nCount, s_PageBucket.nMaxCount);
}

#endif
