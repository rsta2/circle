//
// macaddress.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2025  R. Stange <rsta2@gmx.net>
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
#ifndef _circle_macaddress_h
#define _circle_macaddress_h

#include <circle/string.h>
#include <circle/types.h>

#define MAC_ADDRESS_SIZE	6

class CMACAddress
{
public:
	CMACAddress (void);
	CMACAddress (const u8 *pAddress);
	~CMACAddress (void);

	boolean operator== (const CMACAddress &rAddress2) const;
	boolean operator!= (const CMACAddress &rAddress2) const;
	
	void Set (const u8 *pAddress);
	void SetBroadcast (void);
	void SetMulticast (const u8 *pIPAddress);
	const u8 *Get (void) const;
	void CopyTo (u8 *pBuffer) const;

	boolean IsBroadcast (void) const;
	boolean IsMulticast (void) const;
	unsigned GetSize (void) const;

	void Format (CString *pString) const;
	
private:
	boolean m_bValid;

	u8 m_Address[MAC_ADDRESS_SIZE];
};

#endif
