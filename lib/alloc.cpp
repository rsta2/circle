//
// alloc.cpp
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
#include <circle/alloc.h>
#include <circle/memory.h>
#include <circle/sysconfig.h>
#include <circle/util.h>
#include <assert.h>

void *malloc (size_t nSize)
{
	return CMemorySystem::HeapAllocate (nSize, HEAP_DEFAULT_MALLOC);
}

void *memalign (size_t nAlign, size_t nSize)
{
	assert (nAlign <= HEAP_BLOCK_ALIGN);
	return CMemorySystem::HeapAllocate (nSize, HEAP_DEFAULT_MALLOC);
}

void free (void *pBlock)
{
	CMemorySystem::HeapFree (pBlock);
}

void *calloc (size_t nBlocks, size_t nSize)
{
	nSize *= nBlocks;
	if (nSize == 0)
	{
		nSize = 1;
	}
	assert (nSize >= nBlocks);

	void *pNewBlock = CMemorySystem::HeapAllocate (nSize, HEAP_DEFAULT_MALLOC);
	if (pNewBlock != 0)
	{
		memset (pNewBlock, 0, nSize);
	}

	return pNewBlock;
}

void *realloc (void *pBlock, size_t nSize)
{
	return CMemorySystem::HeapReAllocate (pBlock, nSize);
}

void *palloc (void)
{
	return CMemorySystem::PageAllocate ();
}

void pfree (void *pPage)
{
	CMemorySystem::PageFree (pPage);
}
