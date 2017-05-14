//
// bcmrandom.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_bcmrandom_h
#define _circle_bcmrandom_h

#include <circle/spinlock.h>
#include <circle/types.h>

class CBcmRandomNumberGenerator		/// Driver for the build-in hardware random number generator
{
public:
	CBcmRandomNumberGenerator (void);
	~CBcmRandomNumberGenerator (void);

	/// \return Random number (32-bit)
	u32 GetNumber (void);

private:
	static CSpinLock s_SpinLock;
	static boolean s_bInitialized;
};

#endif
