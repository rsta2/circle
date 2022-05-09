//
// classallocator.h
//
// Class-specific allocator support
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2022  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_classallocator_h
#define _circle_classallocator_h

#include <circle/spinlock.h>
#include <circle/types.h>
#include <assert.h>

// Use the following macros to provide a class-specific allocator to a class:

// goes to the end of a class declaration
#define DECLARE_CLASS_ALLOCATOR							\
	public:									\
		void *operator new (size_t nSize);				\
		void operator delete (void *pBlock, size_t nSize);		\
		static void InitAllocator (unsigned nReservedObjects);		\
		static void InitProtectedAllocator (unsigned nReservedObjects,	\
						    unsigned nTargetLevel);	\
	private:								\
		static CClassAllocator *s_pAllocator;

// goes to the end of a class implementation file
#define IMPLEMENT_CLASS_ALLOCATOR(class)				\
	CClassAllocator *class::s_pAllocator = 0;			\
	void *class::operator new (size_t nSize)			\
	{								\
		assert (nSize == sizeof (class));			\
		assert (s_pAllocator != 0);				\
		return s_pAllocator->Allocate ();			\
	}								\
	void class::operator delete (void *pBlock, size_t nSize)	\
	{								\
		assert (nSize == sizeof (class));			\
		assert (s_pAllocator != 0);				\
		s_pAllocator->Free (pBlock);				\
	}								\
	void class::InitAllocator (unsigned nReservedObjects)		\
	{								\
		assert (s_pAllocator == 0);				\
		s_pAllocator = new CClassAllocator (sizeof (class),	\
						    nReservedObjects,	\
						    #class);		\
		assert (s_pAllocator != 0);				\
	}								\
	void class::InitProtectedAllocator (unsigned nReservedObjects,	\
					    unsigned nTargetLevel)	\
	{								\
		if (s_pAllocator == 0)					\
		{							\
			s_pAllocator = new CClassAllocator (		\
						sizeof (class),		\
						nReservedObjects,	\
						nTargetLevel,		\
						#class);		\
			assert (s_pAllocator != 0);			\
		}							\
		else							\
			s_pAllocator->Extend (nReservedObjects,		\
					      nTargetLevel);		\
	}

// call this somewhere before the class is instantiated
#define INIT_CLASS_ALLOCATOR(class, objects) \
	class::InitAllocator (objects)
// initializes an allocator which is protected by a spin lock
#define INIT_PROTECTED_CLASS_ALLOCATOR(class, objects, level) \
	class::InitProtectedAllocator (objects, level)

class CClassAllocator
{
public:
	CClassAllocator (size_t      nObjectSize,
			 unsigned    nReservedObjects,
			 const char *pClassName);

	CClassAllocator (size_t      nObjectSize,
			 unsigned    nReservedObjects,
			 unsigned    nTargetLevel,
			 const char *pClassName);

	~CClassAllocator (void);

	void *Allocate (void);

	void Free (void *pBlock);

	void Extend (unsigned nReservedObjects, unsigned nTargetLevel);

private:
	void Init (size_t nObjectSize, unsigned nReservedObjects);

private:
	size_t      m_nObjectSize;
	unsigned    m_nReservedObjects;
	const char *m_pClassName;

	unsigned char *m_pMemory;
	struct TBlock *m_pFreeList;

	boolean   m_bProtected;
	unsigned  m_nTargetLevel;
	CSpinLock m_SpinLock;
};

#endif
