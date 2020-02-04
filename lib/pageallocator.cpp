//
// pageallocator.cpp
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
#include <circle/pageallocator.h>
#include <circle/logger.h>
#include <assert.h>

#define PAGE_MASK	(PAGE_SIZE-1)

CPageAllocator::CPageAllocator (void)
:	m_pNext (0),
	m_pLimit (0),
#ifdef PAGE_DEBUG
	m_nCount (0),
	m_nMaxCount (0),
#endif
	m_pFreeList (0)
{
}

CPageAllocator::~CPageAllocator (void)
{
}

void CPageAllocator::Setup (uintptr nBase, size_t nSize)
{
	m_pNext = (u8 *) ((nBase + PAGE_SIZE-1) & ~PAGE_MASK);
	m_pLimit = (u8 *) ((nBase + nSize) & ~PAGE_MASK);
}

size_t CPageAllocator::GetFreeSpace (void) const
{
	return m_pLimit - m_pNext;
}

void *CPageAllocator::Allocate (void)
{
	assert (m_pNext != 0);

	m_SpinLock.Acquire ();

#ifdef PAGE_DEBUG
	if (++m_nCount > m_nMaxCount)
	{
		m_nMaxCount = m_nCount;
	}
#endif

	TFreePage *pFreePage;
	if ((pFreePage = m_pFreeList) != 0)
	{
		assert (pFreePage->nMagic == FREEPAGE_MAGIC);
		m_pFreeList = pFreePage->pNext;
		pFreePage->nMagic = 0;
	}
	else
	{
		pFreePage = (TFreePage *) m_pNext;

		m_pNext += PAGE_SIZE;

		if (m_pNext > m_pLimit)
		{
			m_SpinLock.Release ();

			return 0;		// TODO: system should panic here
		}
	}

	m_SpinLock.Release ();

	return pFreePage;
}

void CPageAllocator::Free (void *pPage)
{
	if (pPage == 0)
	{
		return;
	}

	TFreePage *pFreePage = (TFreePage *) pPage;

	m_SpinLock.Acquire ();

	pFreePage->nMagic = FREEPAGE_MAGIC;

	pFreePage->pNext = m_pFreeList;
	m_pFreeList = pFreePage;

#ifdef PAGE_DEBUG
	m_nCount--;
#endif

	m_SpinLock.Release ();
}

#ifdef PAGE_DEBUG

void CPageAllocator::DumpStatus (void)
{
	CLogger::Get ()->Write ("pager", LogDebug, "%u pages (max %u)", m_nCount, m_nMaxCount);
}

#endif
