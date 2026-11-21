// Stands in for the property tags. It answers PROPTAG_GET_ARM_MEMORY with the
// low memory test.cpp sets in s_nARMMemorySize.
#pragma once
#include <circle/types.h>

#define PROPTAG_GET_ARM_MEMORY	0x00010005

struct TPropertyTagMemory
{
	u32 nBaseAddress;
	u32 nSize;
};

extern u32 s_nARMMemorySize;

class CBcmPropertyTags
{
public:
	CBcmPropertyTags (boolean bEarlyUse = FALSE) {}

	boolean GetTag (u32 nTagId, void *pTag, unsigned nTagSize)
	{
		assert (nTagId == PROPTAG_GET_ARM_MEMORY);
		TPropertyTagMemory *pMemory = (TPropertyTagMemory *) pTag;
		pMemory->nBaseAddress = 0;
		pMemory->nSize = s_nARMMemorySize;

		return TRUE;
	}
};
