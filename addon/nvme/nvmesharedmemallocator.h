//
// nvmesharedmemallocator.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020-2025  R. Stange <rsta2@gmx.net>
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
#ifndef _nvme_nvmesharedmemallocator_h
#define _nvme_nvmesharedmemallocator_h

#include <circle/types.h>

#define NVME_PAGE_SIZE		4096	// The only supported page size for our controller

// We only maintain blocks with the following specification. Other blocks, which do not
// fit these specification (blocks with smaller size/alignment/boundary requirements do
// fit), can be allocated, but are lost, if they are freed (should not happen).
#define NVME_BLOCK_SIZE		NVME_PAGE_SIZE
#define NVME_BLOCK_ALIGN	NVME_PAGE_SIZE
#define NVME_BLOCK_BOUNDARY	0x100000	// Not specified by NVMe spec.

struct TNVMeBlockHeader
{
	u32			 nMagic;
#define NVME_BLOCK_MAGIC	0x4E564D45
	u32			 nSize;
	u32			 nAlign;
	u32			 nBoundary;
	TNVMeBlockHeader	*pNext;
	u8			 Data[0];
};

class CNVMeSharedMemAllocator	/// Shared memory allocation for the NVMe driver
{
public:
	CNVMeSharedMemAllocator (uintptr nMemStart, uintptr nMemEnd);
	~CNVMeSharedMemAllocator (void);

	size_t GetFreeSpace (void) const;

	void *Allocate (size_t nSize, size_t nAlign, size_t nBoundary = NVME_BLOCK_BOUNDARY);

	void Free (void *pBlock);

private:
	uintptr m_nMemStart;
	uintptr m_nMemEnd;

	TNVMeBlockHeader *m_pFreeList;
};

#endif
