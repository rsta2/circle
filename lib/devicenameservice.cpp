//
// devicenameservice.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2015  R. Stange <rsta2@o2online.de>
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
#include <circle/devicenameservice.h>
#include <circle/util.h>
#include <assert.h>

CDeviceNameService *CDeviceNameService::s_This = 0;

CDeviceNameService::CDeviceNameService (void)
:	m_pList (0),
	m_SpinLock (FALSE)
{
	assert (s_This == 0);
	s_This = this;
}

CDeviceNameService::~CDeviceNameService (void)
{
	while (m_pList != 0)
	{
		TDeviceInfo *pNext = m_pList->pNext;

		delete [] m_pList->pName;
		m_pList->pName = 0;
		m_pList->pDevice = 0;
		delete m_pList;

		m_pList = pNext;
	}
	
	s_This = 0;
}

void CDeviceNameService::AddDevice (const char *pName, CDevice *pDevice, boolean bBlockDevice)
{
	m_SpinLock.Acquire ();

	TDeviceInfo *pInfo = new TDeviceInfo;
	assert (pInfo != 0);

	assert (pName != 0);
	pInfo->pName = new char [strlen (pName)+1];
	assert (pInfo->pName != 0);
	strcpy (pInfo->pName, pName);

	assert (pDevice != 0);
	pInfo->pDevice = pDevice;
	
	pInfo->bBlockDevice = bBlockDevice;

	pInfo->pNext = m_pList;
	m_pList = pInfo;

	m_SpinLock.Release ();
}

CDevice *CDeviceNameService::GetDevice (const char *pName, boolean bBlockDevice)
{
	assert (pName != 0);

	m_SpinLock.Acquire ();

	TDeviceInfo *pInfo = m_pList;
	while (pInfo != 0)
	{
		assert (pInfo->pName != 0);
		if (   strcmp (pName, pInfo->pName) == 0
		    && pInfo->bBlockDevice == bBlockDevice)
		{
			CDevice *pResult = pInfo->pDevice;

			m_SpinLock.Release ();

			assert (pResult != 0);
			return pResult;
		}

		pInfo = pInfo->pNext;
	}

	m_SpinLock.Release ();

	return 0;
}

CDeviceNameService *CDeviceNameService::Get (void)
{
	assert (s_This != 0);
	return s_This;
}
