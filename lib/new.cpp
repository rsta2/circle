//
// new.cpp
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
#include <circle/new.h>
#include <assert.h>

void *operator new (size_t nSize, int nType)
{
	return CMemorySystem::HeapAllocate (nSize, nType);
}

void *operator new[] (size_t nSize, int nType)
{
	return CMemorySystem::HeapAllocate (nSize, nType);
}

#if STDLIB_SUPPORT != 3

void *operator new (size_t nSize, void *pMem)
{
	return pMem;
}

void *operator new[] (size_t nSize, void *pMem)
{
	return pMem;
}

void *operator new (size_t nSize)
{
	return CMemorySystem::HeapAllocate (nSize, HEAP_DEFAULT_NEW);
}

void *operator new[] (size_t nSize)
{
	return CMemorySystem::HeapAllocate (nSize, HEAP_DEFAULT_NEW);
}

void operator delete (void *pBlock) noexcept
{
	CMemorySystem::HeapFree (pBlock);
}

void operator delete[] (void *pBlock) noexcept
{
	CMemorySystem::HeapFree (pBlock);
}

void operator delete (void *pBlock, size_t nSize) noexcept
{
	CMemorySystem::HeapFree (pBlock);
}

void operator delete[] (void *pBlock, size_t nSize) noexcept
{
	CMemorySystem::HeapFree (pBlock);
}

// Aligned new and delete (C++17):

#if __cplusplus >= 201703L

void *operator new (size_t nSize, std::align_val_t Align)
{
	assert ((size_t) Align <= HEAP_BLOCK_ALIGN);
	return CMemorySystem::HeapAllocate (nSize, HEAP_DEFAULT_NEW);
}

void *operator new[] (size_t nSize, std::align_val_t Align)
{
	assert ((size_t) Align <= HEAP_BLOCK_ALIGN);
	return CMemorySystem::HeapAllocate (nSize, HEAP_DEFAULT_NEW);
}

void operator delete (void *pBlock, std::align_val_t Align) noexcept
{
	CMemorySystem::HeapFree (pBlock);
}

void operator delete[](void *pBlock, std::align_val_t Align) noexcept
{
	CMemorySystem::HeapFree (pBlock);
}

void operator delete(void *pBlock, size_t nSize, std::align_val_t Align) noexcept
{
	CMemorySystem::HeapFree (pBlock);
}

void operator delete[](void *pBlock, size_t nSize, std::align_val_t Align) noexcept
{
	CMemorySystem::HeapFree (pBlock);
}

#endif

#endif
