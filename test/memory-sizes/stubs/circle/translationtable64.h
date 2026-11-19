// Stands in for the translation table. CMemorySystem creates it only with the
// MMU enabled, which the test does not ask for.
#pragma once
#include <circle/types.h>

class CTranslationTable
{
public:
	CTranslationTable (size_t nMemSize)		{}
	uintptr GetBaseAddress (void) const		{ return 0; }
	void UnmapUnusedPages (size_t nMemSizeHigh)	{}
};
