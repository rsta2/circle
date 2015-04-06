//
// ipaddress.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015  R. Stange <rsta2@o2online.de>
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
#include <circle/net/ipaddress.h>
#include <circle/util.h>
#include <assert.h>

CIPAddress::CIPAddress (void)
:	m_bValid (FALSE)
{
}

CIPAddress::CIPAddress (u32 nAddress)
{
	Set (nAddress);
}

CIPAddress::CIPAddress (const u8 *pAddress)
{
	Set (pAddress);
}

CIPAddress::CIPAddress (const CIPAddress &rAddress)
{
	Set (rAddress.Get ());
}

CIPAddress::~CIPAddress (void)
{
	m_bValid = FALSE;
}

boolean CIPAddress::operator== (const CIPAddress &rAddress2) const
{
	assert (m_bValid);
	return memcmp (m_Address, rAddress2.Get (), IP_ADDRESS_SIZE) == 0 ? TRUE : FALSE;
}

boolean CIPAddress::operator!= (const CIPAddress &rAddress2) const
{
	return !operator== (rAddress2);
}

boolean CIPAddress::operator== (const u8 *pAddress2) const
{
	assert (m_bValid);
	assert (pAddress2 != 0);
	return memcmp (m_Address, pAddress2, IP_ADDRESS_SIZE) == 0 ? TRUE : FALSE;
}

boolean CIPAddress::operator!= (const u8 *pAddress2) const
{
	return !operator== (pAddress2);
}

void CIPAddress::Set (u32 nAddress)
{
	memcpy (m_Address, &nAddress, IP_ADDRESS_SIZE);
	m_bValid = TRUE;
}

void CIPAddress::Set (const u8 *pAddress)
{
	assert (pAddress != 0);
	memcpy (m_Address, pAddress, IP_ADDRESS_SIZE);
	m_bValid = TRUE;
}

void CIPAddress::Set (const CIPAddress &rAddress)
{
	rAddress.CopyTo (m_Address);
	m_bValid = TRUE;
}

const u8 *CIPAddress::Get (void) const
{
	assert (m_bValid);
	return m_Address;
}

void CIPAddress::CopyTo (u8 *pBuffer) const
{
	assert (m_bValid);
	assert (pBuffer != 0);
	memcpy (pBuffer, m_Address, IP_ADDRESS_SIZE);
}

unsigned CIPAddress::GetSize (void) const
{
	return IP_ADDRESS_SIZE;
}

void CIPAddress::Format (CString *pString) const
{
	assert (m_bValid);
	assert (pString != 0);
	pString->Format ("%u.%u.%u.%u",
			(unsigned) m_Address[0], (unsigned) m_Address[1],
			(unsigned) m_Address[2], (unsigned) m_Address[3]);
}

boolean CIPAddress::OnSameNetwork (const CIPAddress &rAddress2, const u8 *pNetMask) const
{
	assert (pNetMask != 0);
	u32 nNetMask = *(u32 *) pNetMask;
	
	u32 nAddress1;
	CopyTo ((u8 *) &nAddress1);
	nAddress1 &= nNetMask;
	
	u32 nAddress2;
	rAddress2.CopyTo ((u8 *) &nAddress2);
	nAddress2 &= nNetMask;

	return nAddress1 == nAddress2 ? TRUE : FALSE;
}
