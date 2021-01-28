//
// xhcisharedmemallocator.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_xhcisharedmemallocator_h
#define _circle_usb_xhcisharedmemallocator_h

#include <circle/types.h>

// Shared memory requirements of this xHCI driver:
//
// Size	Align	Boundary Count		Object
// ---- -----	-------- --------------	-----------------------------------------------
// 16	64	none	 1		ERST
// 248	64	4K	 1		Scatchpad Buffer Array
// 264	64	4K	 1		DCBAA
// 1024	64	64K	 <=2+64*EPs	TRB Ring (EPs = number of endpoints per device)
// 2048	64	4K	 <=64		Device Context
// 124K	4K	4K	 1		Scatchpad Buffers

// We only maintain blocks with the following specification. Other blocks, which do not
// fit these specification (blocks with smaller size/alignment/boundary requirements do
// fit), can be allocated, but are lost, if they are freed (should not happen).
#define XHCI_BLOCK_SIZE		2048
#define XHCI_BLOCK_ALIGN	64
#define XHCI_BLOCK_BOUNDARY	0x10000

struct TXHCIBlockHeader
{
	u32			 nMagic;
#define XHCI_BLOCK_MAGIC	0x58484349
	u32			 nSize;
	u32			 nAlign;
	u32			 nBoundary;
	TXHCIBlockHeader	*pNext;
	u8			 Data[0];
};

class CXHCISharedMemAllocator	/// Shared memory allocation for the xHCI driver
{
public:
	CXHCISharedMemAllocator (uintptr nMemStart, uintptr nMemEnd);
	~CXHCISharedMemAllocator (void);

	size_t GetFreeSpace (void) const;

	void *Allocate (size_t nSize, size_t nAlign, size_t nBoundary);

	void Free (void *pBlock);

private:
	uintptr m_nMemStart;
	uintptr m_nMemEnd;

	TXHCIBlockHeader *m_pFreeList;
};

#endif
