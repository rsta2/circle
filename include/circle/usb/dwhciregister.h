//
// dwhciregister.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#ifndef _dwhciregister_h
#define _dwhciregister_h

#include <circle/usb/dwhci.h>
#include <circle/types.h>

class CDWHCIRegister
{
public:
	CDWHCIRegister (u32 nAddress);
	CDWHCIRegister (u32 nAddress, u32 nValue);
	~CDWHCIRegister (void);

	u32 Read (void);
	void Write (void);

	u32 Get (void) const;
	void Set (u32 nValue);

	boolean IsSet (u32 nMask) const;
	
	void And (u32 nMask);
	void Or (u32 nMask);
	
	void ClearBit (unsigned nBit);
	void SetBit (unsigned nBit);
	void ClearAll (void);
	void SetAll (void);

#ifndef NDEBUG
	void Dump (void) const;
#endif
	
private:
	boolean	m_bValid;
	u32	m_nAddress;
	u32	m_nBuffer;
};

#endif
