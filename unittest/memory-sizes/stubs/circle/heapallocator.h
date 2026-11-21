// Stands in for the heap allocator. It notes the start and the size each heap
// is set up with, so test.cpp can compare them, and it hands out no memory.
#pragma once
#include <circle/types.h>

struct THeapSetup
{
	const char *pName;
	uintptr nBase;
	size_t nSize;
};

// The heaps set up since test.cpp last reset s_nHeapSetups.
extern THeapSetup s_HeapSetups[2];
extern unsigned s_nHeapSetups;

class CHeapAllocator
{
public:
	CHeapAllocator (const char *pName) : m_pName (pName) {}

	void Setup (uintptr nBase, size_t nSize, size_t nReserve)
	{
		assert (s_nHeapSetups < sizeof s_HeapSetups / sizeof s_HeapSetups[0]);
		s_HeapSetups[s_nHeapSetups++] = {m_pName, nBase, nSize};
	}

	void *Allocate (size_t nSize)			{ return 0; }
	void *ReAllocate (void *pBlock, size_t nSize)	{ return 0; }
	void Free (void *pBlock)			{}
	size_t GetFreeSpace (void) const		{ return 0; }

private:
	const char *m_pName;
};
