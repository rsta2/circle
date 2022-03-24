//
// ptrlistfiq.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2022  R. Stange <rsta2@o2online.de>
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
#include <circle/ptrlistfiq.h>
#include <circle/classallocator.h>
#include <circle/synchronize.h>
#include <assert.h>

struct TPtrListElement
{
#ifndef NDEBUG
	unsigned nMagic;
#define PTR_LIST_MAGIC		0x504C4651
#endif
	TPtrListElement *pPrev;
	TPtrListElement *pNext;

	void *pPtr;

	DECLARE_CLASS_ALLOCATOR
};

IMPLEMENT_CLASS_ALLOCATOR (TPtrListElement)

CPtrListFIQ::CPtrListFIQ (unsigned nMaxElements)
:	m_pFirst (0)
{
	INIT_PROTECTED_CLASS_ALLOCATOR (TPtrListElement, nMaxElements, FIQ_LEVEL);
}

CPtrListFIQ::~CPtrListFIQ (void)
{
	assert (m_pFirst == 0);
}

TPtrListElement *CPtrListFIQ::GetFirst (void)
{
	return m_pFirst;
}

TPtrListElement *CPtrListFIQ::GetNext (TPtrListElement *pElement)
{
	assert (pElement != 0);
	assert (pElement->nMagic == PTR_LIST_MAGIC);

	return pElement->pNext;
}

void *CPtrListFIQ::GetPtr (TPtrListElement *pElement)
{
	assert (pElement != 0);
	assert (pElement->nMagic == PTR_LIST_MAGIC);

	return pElement->pPtr;
}

void CPtrListFIQ::InsertBefore (TPtrListElement *pAfter, void *pPtr)
{
	TPtrListElement *pElement = new TPtrListElement;
	assert (pElement != 0);

#ifndef NDEBUG
	pElement->nMagic = PTR_LIST_MAGIC;
#endif
	pElement->pPtr = pPtr;
	
	assert (m_pFirst != 0);
	assert (pAfter != 0);
	assert (pAfter->nMagic == PTR_LIST_MAGIC);

	if (pAfter == m_pFirst)
	{
		pElement->pPrev = 0;
		pElement->pNext = pAfter;

		m_pFirst->pPrev = pElement;

		m_pFirst = pElement;
	}
	else
	{
		pElement->pPrev = pAfter->pPrev;
		pElement->pNext = pAfter;

		if (pAfter->pPrev != 0)
		{
			assert (pAfter->pPrev->nMagic == PTR_LIST_MAGIC);
			pAfter->pPrev->pNext = pElement;
		}

		pAfter->pPrev = pElement;
	}
}

void CPtrListFIQ::InsertAfter (TPtrListElement *pBefore, void *pPtr)
{
	TPtrListElement *pElement = new TPtrListElement;
	assert (pElement != 0);

#ifndef NDEBUG
	pElement->nMagic = PTR_LIST_MAGIC;
#endif
	pElement->pPtr = pPtr;

	if (pBefore == 0)
	{
		assert (m_pFirst == 0);

		pElement->pPrev = 0;
		pElement->pNext = 0;

		m_pFirst = pElement;
	}
	else
	{
		assert (m_pFirst != 0);
		assert (pBefore->nMagic == PTR_LIST_MAGIC);

		pElement->pPrev = pBefore;
		pElement->pNext = pBefore->pNext;

		if (pBefore->pNext != 0)
		{
			assert (pBefore->pNext->nMagic == PTR_LIST_MAGIC);
			pBefore->pNext->pPrev = pElement;
		}

		pBefore->pNext = pElement;
	}
}

void CPtrListFIQ::Remove (TPtrListElement *pElement)
{
	assert (pElement != 0);
	assert (pElement->nMagic == PTR_LIST_MAGIC);

	if (pElement == m_pFirst)
	{
		m_pFirst = pElement->pNext;

		if (pElement->pNext != 0)
		{
			assert (pElement->pNext->nMagic == PTR_LIST_MAGIC);
			pElement->pNext->pPrev = 0;
		}
	}
	else
	{
		assert (pElement->pPrev != 0);
		assert (pElement->pPrev->nMagic == PTR_LIST_MAGIC);
		pElement->pPrev->pNext = pElement->pNext;

		if (pElement->pNext != 0)
		{
			assert (pElement->pNext->nMagic == PTR_LIST_MAGIC);
			pElement->pNext->pPrev = pElement->pPrev;
		}
	}

#ifndef NDEBUG
	pElement->nMagic = 0;
#endif
	delete pElement;
}

TPtrListElement *CPtrListFIQ::Find (void *pPtr)
{
	for (TPtrListElement *pElement = m_pFirst; pElement != 0; pElement = pElement->pNext)
	{
		assert (pElement->nMagic == PTR_LIST_MAGIC);

		if (pElement->pPtr == pPtr)
		{
			return pElement;
		}
	}

	return 0;
}
