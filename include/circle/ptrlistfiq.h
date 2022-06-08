//
// ptrlistfiq.h
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
#ifndef _circle_ptrlistfiq_h
#define _circle_ptrlistfiq_h

struct TPtrListElement;

class CPtrListFIQ				// list of pointers, usable from FIQ_LEVEL
{
public:
	CPtrListFIQ (unsigned nMaxElements);
	~CPtrListFIQ (void);

	TPtrListElement *GetFirst (void);				// returns 0 if list is empty
	TPtrListElement *GetNext (TPtrListElement *pElement);		// returns 0 if nothing follows

	void *GetPtr (TPtrListElement *pElement);			// get pointer for element

	void InsertBefore (TPtrListElement *pAfter, void *pPtr);	// pAfter must be != 0
	void InsertAfter (TPtrListElement *pBefore, void *pPtr);	// pBefore == 0 to set 1st element

	void Remove (TPtrListElement *pElement);			// remove this element

	TPtrListElement *Find (void *pPtr);				// find element using pointer

private:
	TPtrListElement *m_pFirst;
};

#endif
