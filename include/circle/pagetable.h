//
// pagetable.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2019  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_pagetable_h
#define _circle_pagetable_h

#include <circle/types.h>

#if RASPPI <= 3

class CPageTable
{
public:
	CPageTable (u32 nMemSize);
	~CPageTable (void);

	u32 GetBaseAddress (void) const;	// with mode bits to be loaded into TTBRn
	
private:
	u32 *m_pTable;
};

#else

// index into MAIR0 register
#define ATTRINDX_NORMAL		0
#define ATTRINDX_DEVICE		1
#define ATTRINDX_COHERENT	2

class CPageTable		// with LPAE
{
public:
	CPageTable (u32 nMemSize);
	~CPageTable (void);

	u64 GetBaseAddress (void) const;

private:
	u64 *CreateLevel2Table (u64 nBaseAddress);

private:
	u32 m_nMemSize;

	u64 *m_pTable;
};

#endif

#endif
