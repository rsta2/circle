//
// ptrarray.h
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
#ifndef _circle_ptrarray_h
#define _circle_ptrarray_h

class CPtrArray					// dynamic array of pointers
{
public:
	CPtrArray (unsigned nInitialSize = 100, unsigned nSizeIncrement = 100);
	~CPtrArray (void);

	unsigned GetCount (void) const;			// get number of elements in the array

	void *&operator[] (unsigned nIndex);		// get element for index
	void *operator[] (unsigned  nIndex) const;

	unsigned Append (void *pPtr);			// append element to end of array

	void RemoveLast (void);				// remove last element

private:
	unsigned  m_nReservedSize;
	unsigned  m_nSizeIncrement;
	unsigned  m_nUsedCount;
	void	**m_ppArray;
};

#endif
