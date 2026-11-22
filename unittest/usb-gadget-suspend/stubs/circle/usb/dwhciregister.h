#pragma once
#include <circle/types.h>
class CDWHCIRegister
{
      public:
	explicit CDWHCIRegister (u32 address, u32 value = 0) : address (address), value (value)
	{
	}
	void SetAll ()
	{
		value = ~0U;
	}
	u32 Read ();
	void Write ();
	u32 Get () const
	{
		return value;
	}
	void Set (u32 v)
	{
		value = v;
	}
	void And (u32 mask)
	{
		value &= mask;
	}
	void Or (u32 mask)
	{
		value |= mask;
	}
	void Dump () const
	{
	}
	u32 address, value;
};
