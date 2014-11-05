//
// dwhciregister.cpp
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
#include <circle/usb/dwhciregister.h>
#include <circle/memio.h>
#include <circle/logger.h>
#include <assert.h>

CDWHCIRegister::CDWHCIRegister (u32 nAddress)
:	m_bValid (FALSE),
	m_nAddress (nAddress)
{
}

CDWHCIRegister::CDWHCIRegister (u32 nAddress, u32 nValue)
:	m_bValid (TRUE),
	m_nAddress (nAddress),
	m_nBuffer (nValue)
{
}

CDWHCIRegister::~CDWHCIRegister (void)
{
	m_bValid = FALSE;
}

u32 CDWHCIRegister::Read (void)
{
	m_nBuffer = read32 (m_nAddress);
	m_bValid = TRUE;
	
	return m_nBuffer;
}

void CDWHCIRegister::Write (void)
{
	assert (m_bValid);
	write32 (m_nAddress, m_nBuffer);
}

u32 CDWHCIRegister::Get (void) const
{
	assert (m_bValid);
	return m_nBuffer;
}

void CDWHCIRegister::Set (u32 nValue)
{
	m_nBuffer = nValue;
	m_bValid = TRUE;
}

boolean CDWHCIRegister::IsSet (u32 nMask) const
{
	assert (m_bValid);
	return m_nBuffer & nMask ? TRUE : FALSE;
}

void CDWHCIRegister::And (u32 nMask)
{
	assert (m_bValid);
	m_nBuffer &= nMask;
}

void CDWHCIRegister::Or (u32 nMask)
{
	assert (m_bValid);
	m_nBuffer |= nMask;
}

void CDWHCIRegister::ClearBit (unsigned nBit)
{
	assert (m_bValid);
	assert (nBit < sizeof m_nBuffer * 8);
	m_nBuffer &= ~(1 << nBit);
}

void CDWHCIRegister::SetBit (unsigned nBit)
{
	assert (m_bValid);
	assert (nBit < sizeof m_nBuffer * 8);
	m_nBuffer |= 1 << nBit;
}

void CDWHCIRegister::ClearAll (void)
{
	m_nBuffer = 0;
	m_bValid = TRUE;
}

void CDWHCIRegister::SetAll (void)
{
	m_nBuffer = (u32) -1;
	m_bValid = TRUE;
}

#ifndef NDEBUG

void CDWHCIRegister::Dump (void) const
{
	if (m_bValid)
	{
		CLogger::Get ()->Write ("dwhci", LogDebug,
					"Register at 0x%X is 0x%X",
					m_nAddress & 0xFFF, m_nBuffer);
	}
	else
	{
		CLogger::Get ()->Write ("dwhci", LogDebug,
					"Register at 0x%X was not set",
					m_nAddress & 0xFFF);
	}
}

#endif
