//
// pagetable.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#ifndef _pagetable_h
#define _pagetable_h

#include <circle/armv6mmu.h>
#include <circle/types.h>

class CPageTable
{
public:
	// 4GB shared device
	CPageTable (void);

	// 0..nMemSize: normal,
	// nMemSize..512MB: shared device (1024MB on Raspberry Pi 2)
	CPageTable (u32 nMemSize);

	~CPageTable (void);

	u32 GetBaseAddress (void) const;	// with mode bits to be loaded into TTBRn
	
private:
	boolean m_bTableAllocated;
	TARMV6MMU_LEVEL1_SECTION_DESCRIPTOR *m_pTable;
};

#endif
