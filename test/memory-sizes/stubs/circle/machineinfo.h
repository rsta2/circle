// Stands in for the machine info. It reports the RAM size test.cpp sets in
// s_nRAMSize. GetRAMSizeEarly () returns 2048 from RASPPI 5 on, as
// lib/machineinfo.cpp does, which cannot read the size that early there.
#pragma once
#include <circle/types.h>

extern unsigned s_nRAMSize;

class CMachineInfo
{
public:
	static CMachineInfo *Get (void)
	{
		static CMachineInfo Info;
		return &Info;
	}

	unsigned GetRAMSize (void) const	{ return s_nRAMSize; }

	static unsigned GetRAMSizeEarly (void)
	{
#if RASPPI >= 5
		return 2048;
#else
		return s_nRAMSize;
#endif
	}
};
