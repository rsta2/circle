//
// device.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2023  R. Stange <rsta2@o2online.de>
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

struct TRemovedHandlerEntry
{
	TDeviceRemovedHandler *pHandler;
	void		      *pContext;
};

CDevice::CDevice (void)
{
}

CDevice::~CDevice (void)
{
	TPtrListElement *pElement;
	while ((pElement = m_RemovedHandlerList.GetFirst ()) != 0)
	{
		TRemovedHandlerEntry *pEntry =
			(TRemovedHandlerEntry *) m_RemovedHandlerList.GetPtr (pElement);

		assert (pEntry->pHandler != 0);
		(*pEntry->pHandler) (this, pEntry->pContext);

		m_RemovedHandlerList.Remove (pElement);

		delete pEntry;
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

u64 CDevice::GetSize (void) const
{
	return (u64) -1;
}

int CDevice::IOCtl (unsigned long ulCmd, void *pData)
{
	return -1;
}

boolean CDevice::RemoveDevice (void)
{
	return FALSE;
}

CDevice::TRegistrationHandle CDevice::RegisterRemovedHandler (TDeviceRemovedHandler *pHandler,
							      void *pContext)
{
	assert (pHandler != 0);		// not allowed any more to unregister

	TRemovedHandlerEntry *pEntry = new TRemovedHandlerEntry;
	assert (pEntry != 0);
	pEntry->pHandler = pHandler;
	pEntry->pContext = pContext;

	TPtrListElement *pElement = m_RemovedHandlerList.GetFirst ();
	if (pElement == 0)
	{
		m_RemovedHandlerList.InsertAfter (0, pEntry);
	}
	else
	{
		m_RemovedHandlerList.InsertBefore (pElement, pEntry);
	}

	return (TRegistrationHandle) pEntry;
}

void CDevice::UnregisterRemovedHandler (TRegistrationHandle hRegistration)
{
	TRemovedHandlerEntry *pEntry = (TRemovedHandlerEntry *) hRegistration;
	assert (pEntry != 0);

	TPtrListElement *pElement = m_RemovedHandlerList.Find (pEntry);
	assert (pElement != 0);

	m_RemovedHandlerList.Remove (pElement);

	delete pEntry;
}
