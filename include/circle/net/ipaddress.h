//
// ipaddress.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_net_ipaddress_h
#define _circle_net_ipaddress_h

#include <circle/string.h>
#include <circle/types.h>

#define IP_ADDRESS_SIZE	4

class CIPAddress
{
public:
	CIPAddress (void);
	CIPAddress (u32 nAddress);
	CIPAddress (const u8 *pAddress);
	CIPAddress (const CIPAddress &rAddress);
	~CIPAddress (void);

	boolean operator== (const CIPAddress &rAddress2) const;
	boolean operator!= (const CIPAddress &rAddress2) const;
	boolean operator== (const u8 *pAddress2) const;
	boolean operator!= (const u8 *pAddress2) const;
	boolean operator== (u32 nAddress2) const;
	boolean operator!= (u32 nAddress2) const;

	CIPAddress &operator= (u32 nAddress);
	void Set (u32 nAddress);
	void Set (const u8 *pAddress);
	void Set (const CIPAddress &rAddress);
	void SetBroadcast (void);

	operator u32 (void) const;
	const u8 *Get (void) const;
	void CopyTo (u8 *pBuffer) const;

	boolean IsNull (void) const;
	boolean IsBroadcast (void) const;
	unsigned GetSize (void) const;

	void Format (CString *pString) const;
	
	boolean OnSameNetwork (const CIPAddress &rAddress2, const u8 *pNetMask) const;

private:
#ifndef NDEBUG
	boolean m_bValid;
#endif

	u32 m_nAddress;
};

#endif
