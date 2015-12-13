//
// ptrarray.cpp
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
#include <circle/ptrarray.h>
#include <circle/util.h>
#include <assert.h>

CPtrArray::CPtrArray (unsigned nInitialSize, unsigned nSizeIncrement)
:	m_nReservedSize (nInitialSize),
	m_nSizeIncrement (nSizeIncrement),
	m_nUsedCount (0),
	m_ppArray (0)
{
	assert (m_nReservedSize > 0);
	assert (m_nSizeIncrement > 0);

	m_ppArray = new void * [m_nReservedSize];
	assert (m_ppArray != 0);
}

CPtrArray::~CPtrArray (void)
{
	m_nReservedSize = 0;
	m_nSizeIncrement = 0;

	delete [] m_ppArray;
	m_ppArray = 0;
}

unsigned CPtrArray::GetCount (void) const
{
	return m_nUsedCount;
}

void *&CPtrArray::operator[] (unsigned nIndex)
{
	assert (nIndex < m_nUsedCount);
	assert (m_nUsedCount <= m_nReservedSize);
	assert (m_ppArray != 0);

	return m_ppArray[nIndex];
}

void *CPtrArray::operator[] (unsigned  nIndex) const
{
	assert (nIndex < m_nUsedCount);
	assert (m_nUsedCount <= m_nReservedSize);
	assert (m_ppArray != 0);

	return m_ppArray[nIndex];
}

unsigned CPtrArray::Append (void *pPtr)
{
	assert (m_nReservedSize > 0);
	assert (m_ppArray != 0);

	assert (m_nUsedCount <= m_nReservedSize);
	if (m_nUsedCount == m_nReservedSize)
	{
		assert (m_nSizeIncrement > 0);
		void **ppNewArray = new void * [m_nReservedSize + m_nSizeIncrement];
		assert (ppNewArray != 0);

		memcpy (ppNewArray, m_ppArray, m_nReservedSize * sizeof (void *));

		delete [] m_ppArray;
		m_ppArray = ppNewArray;

		m_nReservedSize += m_nSizeIncrement;
	}

	m_ppArray[m_nUsedCount] = pPtr;

	return m_nUsedCount++;
}

void CPtrArray::RemoveLast (void)
{
	assert (m_nUsedCount > 0);
	m_nUsedCount--;
}
