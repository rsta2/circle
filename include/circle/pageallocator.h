//
// pageallocator.h
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
#ifndef _circle_pageallocator_h
#define _circle_pageallocator_h

#include <circle/sysconfig.h>
#include <circle/spinlock.h>
#include <circle/macros.h>
#include <circle/types.h>

//#define PAGE_DEBUG

struct TFreePage
{
	u32		 nMagic;
#define FREEPAGE_MAGIC	0x50474D43
	TFreePage	*pNext;
};

class CPageAllocator	/// Allocates aligned pages from a flat memory region
{
public:
	CPageAllocator (void);
	~CPageAllocator (void);

	/// \param nBase Base address of memory region
	/// \param nSize Size of memory region
	void Setup (uintptr nBase, size_t nSize) NOOPT;

	/// \return Free space of the memory region, which is not allocated by pages
	/// \note Unused pages on the free list do not count here.
	size_t GetFreeSpace (void) const;

	/// \return Pointer to a page with a size of PAGE_SIZE
	/// \note Resulting page is always aligned to PAGE_SIZE
	void *Allocate (void);

	/// \param pPage Memory page to be freed
	void Free (void *pPage);

#ifdef PAGE_DEBUG
	void DumpStatus (void);
#endif

private:
	u8		*m_pNext;
	u8		*m_pLimit;
#ifdef PAGE_DEBUG
	unsigned	 m_nCount;
	unsigned	 m_nMaxCount;
#endif
	TFreePage	*m_pFreeList;
	CSpinLock	 m_SpinLock;
};

#endif
