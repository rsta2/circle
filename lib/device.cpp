//
// device.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#include <circle/device.h>

CDevice::CDevice (void)
:	m_pRemovedHandler (0)
{
}

CDevice::~CDevice (void)
{
	if (m_pRemovedHandler != 0)
	{
		(*m_pRemovedHandler) (this, m_pRemovedContext);

		m_pRemovedHandler = 0;
	}
}

int CDevice::Read (void *pBuffer, size_t nCount)
{
	return -1;
}

int CDevice::Write (const void *pBuffer, size_t nCount)
{
	return -1;
}

u64 CDevice::Seek (u64 ullOffset)
{
	return (u64) -1;
}

boolean CDevice::RemoveDevice (void)
{
	return FALSE;
}

void CDevice::RegisterRemovedHandler (TDeviceRemovedHandler *pHandler, void *pContext)
{
	m_pRemovedContext = pContext;
	m_pRemovedHandler = pHandler;
}
