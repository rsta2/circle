//
// macaddress.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2019  R. Stange <rsta2@o2online.de>
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
#include <circle/macaddress.h>
#include <circle/util.h>
#include <assert.h>

CMACAddress::CMACAddress (void)
:	m_bValid (FALSE)
{
}

CMACAddress::CMACAddress (const u8 *pAddress)
{
	Set (pAddress);
}

CMACAddress::~CMACAddress (void)
{
	m_bValid = FALSE;
}

boolean CMACAddress::operator== (const CMACAddress &rAddress2) const
{
	assert (m_bValid);
	return memcmp (m_Address, rAddress2.Get (), MAC_ADDRESS_SIZE) == 0 ? TRUE : FALSE;
}

boolean CMACAddress::operator!= (const CMACAddress &rAddress2) const
{
	return !operator== (rAddress2);
}

void CMACAddress::Set (const u8 *pAddress)
{
	assert (pAddress != 0);
	memcpy (m_Address, pAddress, MAC_ADDRESS_SIZE);
	m_bValid = TRUE;
}

void CMACAddress::SetBroadcast (void)
{
	memset (m_Address, 0xFF, MAC_ADDRESS_SIZE);
	m_bValid = TRUE;
}

const u8 *CMACAddress::Get (void) const
{
	assert (m_bValid);
	return m_Address;
}

void CMACAddress::CopyTo (u8 *pBuffer) const
{
	assert (m_bValid);
	assert (pBuffer != 0);
	memcpy (pBuffer, m_Address, MAC_ADDRESS_SIZE);
}

boolean CMACAddress::IsBroadcast (void) const
{
	assert (m_bValid);

	for (unsigned i = 0; i < MAC_ADDRESS_SIZE; i++)
	{
		if (m_Address[i] != 0xFF)
		{
			return FALSE;
		}
	}

	return TRUE;
}

unsigned CMACAddress::GetSize (void) const
{
	return MAC_ADDRESS_SIZE;
}

void CMACAddress::Format (CString *pString) const
{
	assert (m_bValid);
	assert (pString != 0);
	pString->Format ("%02X:%02X:%02X:%02X:%02X:%02X",
			(unsigned) m_Address[0], (unsigned) m_Address[1],
			(unsigned) m_Address[2], (unsigned) m_Address[3],
			(unsigned) m_Address[4], (unsigned) m_Address[5]);
}
