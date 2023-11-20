//
// ptrlist.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2023  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_ptrlist_h
#define _circle_ptrlist_h

struct TPtrListElement;

class CPtrList		/// List of pointers
{
public:
	CPtrList (void);
	~CPtrList (void);

	/// \return Pointer to first element in the list (nullptr, if list is empty)
	TPtrListElement *GetFirst (void) const;
	/// \param pElement Pointer to current element in the list
	/// \return Pointer to next element in the list following pElement\n
	///	    (nullptr, if nothing follows)
	TPtrListElement *GetNext (TPtrListElement *pElement) const;

	/// \param pElement Pointer to an element in a list
	/// \return The user pointer of this element
	static void *GetPtr (TPtrListElement *pElement);

	/// \brief Insert an element before the given element
	/// \param pAfter Pointer to the element, which will follow inserted element (not nullptr)
	/// \param pPtr The user pointer to be saved in the element
	void InsertBefore (TPtrListElement *pAfter, void *pPtr);
	/// \brief Insert an element after the given element
	/// \param pBefore Pointer to the element, which will precede inserted element\n
	///		   (nullptr for the first element in the list)
	/// \param pPtr The user pointer to be saved in the element
	void InsertAfter (TPtrListElement *pBefore, void *pPtr);

	/// \brief Remove this element from the list
	/// \param pElement Pointer to the element to be removed
	void Remove (TPtrListElement *pElement);

	/// \brief Find an element in the list using the user pointer
	/// \param pPtr User pointer to find searched for
	/// \return First element in list, which has this user pointer (or nullptr, if not found)
	TPtrListElement *Find (void *pPtr) const;

private:
	TPtrListElement *m_pFirst;
};

#endif
