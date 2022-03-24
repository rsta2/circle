//
// classallocator.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2022  R. Stange <rsta2@o2online.de>
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
#include <circle/classallocator.h>
#include <circle/alloc.h>
#include <circle/logger.h>

#define BLOCK_ALIGN	16U
#define ALIGN_MASK	(~(BLOCK_ALIGN-1))

struct TBlock
{
	unsigned       nMagic;
#define BLOCK_MAGIC	0x4F425350U
	unsigned       nPadding;
#if AARCH == 32
	unsigned       nPadding2;
#endif
	TBlock        *pNext;
	unsigned char  Data[0];
};

CClassAllocator::CClassAllocator (size_t      nObjectSize,
				  unsigned    nReservedObjects,
				  const char *pClassName)
:	m_pClassName (pClassName),
	m_pMemory (0),
	m_pFreeList (0),
	m_bProtected (FALSE)
{
	Init (nObjectSize, nReservedObjects);
}

CClassAllocator::CClassAllocator (size_t      nObjectSize,
				  unsigned    nReservedObjects,
				  unsigned    nTargetLevel,
				  const char *pClassName)
:	m_pClassName (pClassName),
	m_pMemory (0),
	m_pFreeList (0),
	m_bProtected (TRUE),
	m_nTargetLevel (nTargetLevel),
	m_SpinLock (nTargetLevel)
{
	Init (nObjectSize, nReservedObjects);
}

// TODO: Does not free all memory, if class store has been extended.
CClassAllocator::~CClassAllocator (void)
{
	m_pFreeList = 0;

	if (m_pMemory != 0)
	{
		free (m_pMemory);
	}

	m_pMemory = 0;
}

void CClassAllocator::Init (size_t nObjectSize, unsigned nReservedObjects)
{
	assert (sizeof (TBlock) == BLOCK_ALIGN);

	if (nObjectSize == 0)
	{
		nObjectSize = 1;
	}
	m_nObjectSize = (nObjectSize + sizeof (TBlock) + BLOCK_ALIGN-1) & ALIGN_MASK;

	assert (nReservedObjects > 0);
	m_nReservedObjects = nReservedObjects;

	m_pMemory = reinterpret_cast<unsigned char *> (malloc (m_nObjectSize*m_nReservedObjects));
	if (m_pMemory == 0)
	{
		m_nReservedObjects = 0;

		return;
	}
	assert ((reinterpret_cast<uintptr> (m_pMemory) & ~ALIGN_MASK) == 0);

	for (unsigned i = 0; i < m_nReservedObjects; i++)
	{
		TBlock *pBlock = reinterpret_cast<TBlock *> (m_pMemory + m_nObjectSize*i);

		pBlock->nMagic = BLOCK_MAGIC;
		pBlock->pNext = m_pFreeList;

		m_pFreeList = pBlock;
	}
}

void CClassAllocator::Extend (unsigned nReservedObjects, unsigned nTargetLevel)
{
	assert (m_bProtected);
	assert (m_nTargetLevel == nTargetLevel);
	assert (nReservedObjects > 0);

	unsigned char *pMemory = reinterpret_cast<unsigned char *> (malloc (  m_nObjectSize
									    * nReservedObjects));
	if (pMemory == 0)
	{
		return;
	}
	assert ((reinterpret_cast<uintptr> (pMemory) & ~ALIGN_MASK) == 0);

	m_SpinLock.Acquire ();

	for (unsigned i = 0; i < nReservedObjects; i++)
	{
		TBlock *pBlock = reinterpret_cast<TBlock *> (pMemory + m_nObjectSize*i);

		pBlock->nMagic = BLOCK_MAGIC;
		pBlock->pNext = m_pFreeList;

		m_pFreeList = pBlock;
	}

	m_nReservedObjects += nReservedObjects;

	m_SpinLock.Release ();
}

void *CClassAllocator::Allocate (void)
{
	if (m_bProtected)
	{
		m_SpinLock.Acquire ();
	}

	TBlock *pBlock = m_pFreeList;
	if (pBlock == 0)
	{
		if (m_bProtected)
		{
			m_SpinLock.Release ();
		}

		CLogger::Get ()->Write (m_pClassName, LogPanic,
					"Trying to allocate more than %u instances",
					m_nReservedObjects);

		return 0;
	}

	assert (pBlock->nMagic == BLOCK_MAGIC);
	m_pFreeList = pBlock->pNext;
	pBlock->pNext = 0;

	if (m_bProtected)
	{
		m_SpinLock.Release ();
	}

	return pBlock->Data;
}

void CClassAllocator::Free (void *pBlock)
{
	assert (pBlock != 0);
	TBlock *pBlk = reinterpret_cast<TBlock *> (
		reinterpret_cast<unsigned char *> (pBlock)-sizeof (TBlock));

	assert (pBlk->nMagic == BLOCK_MAGIC);
	assert (pBlk->pNext == 0);

	if (m_bProtected)
	{
		m_SpinLock.Acquire ();
	}

	pBlk->pNext = m_pFreeList;
	m_pFreeList = pBlk;

	if (m_bProtected)
	{
		m_SpinLock.Release ();
	}
}
