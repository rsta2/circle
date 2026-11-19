// Stands in for the page allocator, which the test does not use.
#pragma once
#include <circle/types.h>

class CPageAllocator
{
public:
	void Setup (uintptr nBase, size_t nSize)	{}
	void *Allocate (void)				{ return 0; }
	void Free (void *pPage)				{}
};
