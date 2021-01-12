//
// devicenameservice.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2018  R. Stange <rsta2@o2online.de>
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
#include <circle/string.h>
#include <circle/util.h>
#include <assert.h>

CDeviceNameService *CDeviceNameService::s_This = 0;

CDeviceNameService::CDeviceNameService (void)
:	m_pList (0),
	m_SpinLock (TASK_LEVEL)
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

void CDeviceNameService::AddDevice (const char *pPrefix, unsigned nIndex,
				    CDevice *pDevice, boolean bBlockDevice)
{
	CString Name;
	Name.Format ("%s%u", pPrefix, nIndex);

	AddDevice (Name, pDevice, bBlockDevice);
}

void CDeviceNameService::RemoveDevice (const char *pName, boolean bBlockDevice)
{
	assert (pName != 0);

	m_SpinLock.Acquire ();

	TDeviceInfo *pInfo = m_pList;
	TDeviceInfo *pPrev = 0;
	while (pInfo != 0)
	{
		assert (pInfo->pName != 0);
		if (   strcmp (pName, pInfo->pName) == 0
		    && pInfo->bBlockDevice == bBlockDevice)
		{
			break;
		}

		pPrev = pInfo;
		pInfo = pInfo->pNext;
	}

	if (pInfo == 0)
	{
		m_SpinLock.Release ();

		return;
	}

	if (pPrev == 0)
	{
		m_pList = pInfo->pNext;
	}
	else
	{
		pPrev->pNext = pInfo->pNext;
	}

	m_SpinLock.Release ();

	delete [] pInfo->pName;
	pInfo->pName = 0;
	pInfo->pDevice = 0;
	delete pInfo;
}

void CDeviceNameService::RemoveDevice (const char *pPrefix, unsigned nIndex, boolean bBlockDevice)
{
	CString Name;
	Name.Format ("%s%u", pPrefix, nIndex);

	RemoveDevice (Name, bBlockDevice);
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

CDevice *CDeviceNameService::GetDevice (const char *pPrefix, unsigned nIndex, boolean bBlockDevice)
{
	CString Name;
	Name.Format ("%s%u", pPrefix, nIndex);

	return GetDevice (Name, bBlockDevice);
}

boolean CDeviceNameService::EnumerateDevices (
	boolean (*callback)(CDevice* pDevice, const char* name, boolean bBlockDevice, void* arg), 
	void* arg
	)
{
	m_SpinLock.Acquire ();

	boolean result = true;
	TDeviceInfo *pInfo = m_pList;
	while (pInfo != 0)
	{
		if (!callback(pInfo->pDevice, pInfo->pName, pInfo->bBlockDevice, arg))
		{
			result = false;
			break;
		}
		
		pInfo = pInfo->pNext;
	}

	m_SpinLock.Release ();
	return result;
}


void CDeviceNameService::ListDevices (CDevice *pTarget)
{
	assert (pTarget != 0);

	unsigned i = 0;

	TDeviceInfo *pInfo = m_pList;
	while (pInfo != 0)
	{
		CString String;

		assert (pInfo->pName != 0);
		String.Format ("%c %-12s%c",
			       pInfo->bBlockDevice ? 'b' : 'c',
			       (const char *) pInfo->pName,
			       ++i % 4 == 0 ? '\n' : ' ');

		pTarget->Write ((const char *) String, String.GetLength ());

		pInfo = pInfo->pNext;
	}

	if (i % 4 != 0)
	{
		pTarget->Write ("\n", 1);
	}
}

CDeviceNameService *CDeviceNameService::Get (void)
{
	assert (s_This != 0);
	return s_This;
}
