//
// dwhcicompletionqueue.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2022  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/dwhcicompletionqueue.h>
#include <assert.h>

#ifdef USE_USB_FIQ

CDWHCICompletionQueue::CDWHCICompletionQueue (unsigned nMaxElements)
:	m_List (nMaxElements),
	m_SpinLock (FIQ_LEVEL)
{
}

CDWHCICompletionQueue::~CDWHCICompletionQueue (void)
{
	CUSBRequest *pURB;
	while ((pURB = Dequeue()) != 0)
	{
		delete pURB;
	}
}

boolean CDWHCICompletionQueue::IsEmpty (void)
{
	return !m_List.GetFirst ();
}

void CDWHCICompletionQueue::Enqueue (CUSBRequest *pURB)
{
	assert (pURB != 0);

	m_SpinLock.Acquire ();

	TPtrListElement *pPrevElement = 0;
	TPtrListElement *pElement = m_List.GetFirst ();
	while (pElement != 0)
	{
		pPrevElement = pElement;
		pElement = m_List.GetNext (pElement);
	}

	m_List.InsertAfter (pPrevElement, pURB);

	m_SpinLock.Release ();
}

CUSBRequest *CDWHCICompletionQueue::Dequeue (void)
{
	m_SpinLock.Acquire ();

	TPtrListElement *pElement = m_List.GetFirst ();
	if (pElement == 0)
	{
		m_SpinLock.Release ();

		return 0;
	}

	CUSBRequest *pURB = (CUSBRequest *) m_List.GetPtr (pElement);
	assert (pURB != 0);

	m_List.Remove (pElement);

	m_SpinLock.Release ();

	return pURB;
}

#endif
