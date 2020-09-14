//
// numberpool.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_numberpool_h
#define _circle_numberpool_h

#include <circle/types.h>

class CNumberPool	/// Allocation pool for (device) numbers
{
public:
	static const unsigned Limit   = 63;		/// Allowed maximum
	static const unsigned Invalid = Limit+1;

public:
	/// \param nMin Minimal returned number (normally 0 or 1)
	/// \param nMax Maximal returned number
	CNumberPool (unsigned nMin, unsigned nMax = Limit);

	~CNumberPool (void);

	/// \param bMustSucceed System will panic if this is TRUE and allocation would fail.
	/// \param pFrom From string for panic message
	/// \return Allocated number, "Invalid" on failure (if bMustSucceed is FALSE)
	unsigned AllocateNumber (boolean bMustSucceed, const char *pFrom = "numpool");

	/// \param nNumber Allocated number to be freed for reuse
	void FreeNumber (unsigned nNumber);

private:
	unsigned m_nMin;
	unsigned m_nMax;

	u64 m_nMap;
};

#endif
