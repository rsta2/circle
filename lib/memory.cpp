//
// memory.cpp
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
#include <circle/memory.h>
#include <circle/bcmpropertytags.h>
#include <circle/alloc.h>
#include <circle/sysconfig.h>
#include <assert.h>

CMemorySystem::CMemorySystem (void)
:	m_nMemSize (0)
{
	CBcmPropertyTags Tags;
	TPropertyTagMemory TagMemory;
	if (!Tags.GetTag (PROPTAG_GET_ARM_MEMORY, &TagMemory, sizeof TagMemory))
	{
		TagMemory.nBaseAddress = 0;
		TagMemory.nSize = ARM_MEM_SIZE;
	}

	assert (TagMemory.nBaseAddress == 0);
	m_nMemSize = TagMemory.nSize;

	mem_init (TagMemory.nBaseAddress, m_nMemSize);
}

CMemorySystem::~CMemorySystem (void)
{
}

u32 CMemorySystem::GetMemSize (void) const
{
	return m_nMemSize;
}
