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
#include <circle/net/ipaddress.h>
#include <circle/util.h>
#include <assert.h>

CIPAddress::CIPAddress (void)
#ifndef NDEBUG
:	m_bValid (FALSE)
#endif
{
}

CIPAddress::CIPAddress (u32 nAddress)
{
	m_nAddress = nAddress;
#ifndef NDEBUG
	m_bValid = TRUE;
#endif
}

CIPAddress::CIPAddress (const u8 *pAddress)
{
	Set (pAddress);
}

CIPAddress::CIPAddress (const CIPAddress &rAddress)
{
	assert (rAddress.m_bValid);
	m_nAddress = rAddress.m_nAddress;
#ifndef NDEBUG
	m_bValid = TRUE;
#endif
}

CIPAddress::~CIPAddress (void)
{
#ifndef NDEBUG
	m_bValid = FALSE;
#endif
}

boolean CIPAddress::operator== (const CIPAddress &rAddress2) const
{
	assert (m_bValid);
	assert (rAddress2.m_bValid);
	return m_nAddress == rAddress2.m_nAddress;
}

boolean CIPAddress::operator!= (const CIPAddress &rAddress2) const
{
	assert (m_bValid);
	assert (rAddress2.m_bValid);
	return m_nAddress != rAddress2.m_nAddress;
}

boolean CIPAddress::operator== (const u8 *pAddress2) const
{
	assert (m_bValid);
	assert (pAddress2 != 0);
	return memcmp (&m_nAddress, pAddress2, IP_ADDRESS_SIZE) == 0 ? TRUE : FALSE;
}

boolean CIPAddress::operator!= (const u8 *pAddress2) const
{
	return !operator== (pAddress2);
}

boolean CIPAddress::operator== (u32 nAddress2) const
{
	assert (m_bValid);
	return m_nAddress == nAddress2;
}

boolean CIPAddress::operator!= (u32 nAddress2) const
{
	assert (m_bValid);
	return m_nAddress != nAddress2;
}

CIPAddress &CIPAddress::operator= (u32 nAddress)
{
	m_nAddress = nAddress;
#ifndef NDEBUG
	m_bValid = TRUE;
#endif
	return *this;
}

void CIPAddress::Set (u32 nAddress)
{
	m_nAddress = nAddress;
#ifndef NDEBUG
	m_bValid = TRUE;
#endif
}

void CIPAddress::Set (const u8 *pAddress)
{
	assert (pAddress != 0);
	memcpy (&m_nAddress, pAddress, IP_ADDRESS_SIZE);
#ifndef NDEBUG
	m_bValid = TRUE;
#endif
}

void CIPAddress::Set (const CIPAddress &rAddress)
{
	assert (rAddress.m_bValid);
	m_nAddress = rAddress.m_nAddress;
#ifndef NDEBUG
	m_bValid = TRUE;
#endif
}

void CIPAddress::SetBroadcast (void)
{
	m_nAddress = 0xFFFFFFFF;
#ifndef NDEBUG
	m_bValid = TRUE;
#endif
}

CIPAddress::operator u32 (void) const
{
	assert (m_bValid);
	return m_nAddress;
}

const u8 *CIPAddress::Get (void) const
{
	assert (m_bValid);
	return (const u8 *) &m_nAddress;
}

void CIPAddress::CopyTo (u8 *pBuffer) const
{
	assert (m_bValid);
	assert (pBuffer != 0);
	memcpy (pBuffer, &m_nAddress, IP_ADDRESS_SIZE);
}

boolean CIPAddress::IsNull (void) const
{
	assert (m_bValid);
	return m_nAddress == 0;
}

boolean CIPAddress::IsBroadcast (void) const
{
	assert (m_bValid);
	return m_nAddress == 0xFFFFFFFF;
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
			m_nAddress & 0xFF, m_nAddress >> 8 & 0xFF,
			m_nAddress >> 16 & 0xFF, m_nAddress >> 24 & 0xFF);
}

boolean CIPAddress::OnSameNetwork (const CIPAddress &rAddress2, const u8 *pNetMask) const
{
	if (rAddress2.IsBroadcast ())	// a broadcast goes always to the same subnet
	{
		return TRUE;
	}

	assert (pNetMask != 0);
	u32 nNetMask;
	memcpy (&nNetMask, pNetMask, IP_ADDRESS_SIZE);

	assert (m_bValid);
	u32 nAddress1 = m_nAddress;
	nAddress1 &= nNetMask;

	u32 nAddress2 = rAddress2.m_nAddress;
	nAddress2 &= nNetMask;

	return nAddress1 == nAddress2 ? TRUE : FALSE;
}
