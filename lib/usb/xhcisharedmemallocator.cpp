//
// xhcisharedmemallocator.cpp
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
#include <circle/usb/xhcisharedmemallocator.h>
#include <circle/logger.h>
#include <assert.h>

static const char From[] = "xhcialloc";

CXHCISharedMemAllocator::CXHCISharedMemAllocator (uintptr nMemStart, uintptr nMemEnd)
:	m_nMemStart (nMemStart),
	m_nMemEnd (nMemEnd),
	m_pFreeList (0)
{
	assert (m_nMemStart != 0);
	assert (m_nMemEnd > m_nMemStart);
}

CXHCISharedMemAllocator::~CXHCISharedMemAllocator (void)
{
	m_nMemStart = 0;
	m_nMemEnd = 0;

	m_pFreeList = 0;
}

size_t CXHCISharedMemAllocator::GetFreeSpace (void) const
{
	return m_nMemEnd > m_nMemStart ? m_nMemEnd - m_nMemStart : 0;
}

void *CXHCISharedMemAllocator::Allocate (size_t nSize, size_t nAlign, size_t nBoundary)
{
	assert (nSize > 0);
	assert (nAlign != 0);
	assert (nAlign <= nBoundary);
	assert (m_nMemStart != 0);
	assert (m_nMemEnd != 0);

	if (   nSize <= XHCI_BLOCK_SIZE
	    && nAlign <= XHCI_BLOCK_ALIGN
	    && nBoundary <= XHCI_BLOCK_BOUNDARY)
	{
		if (m_pFreeList != 0)
		{
			TXHCIBlockHeader *pBlockHeader = m_pFreeList;
			assert (pBlockHeader->nMagic == XHCI_BLOCK_MAGIC);
			assert (pBlockHeader->nSize == XHCI_BLOCK_SIZE);
			assert (pBlockHeader->nAlign == XHCI_BLOCK_ALIGN);
			assert (pBlockHeader->nBoundary == XHCI_BLOCK_BOUNDARY);

			m_pFreeList = pBlockHeader->pNext;
			pBlockHeader->pNext = 0;

			void *pResult = pBlockHeader->Data;
			assert (((uintptr) pResult & (XHCI_BLOCK_ALIGN-1)) == 0);

			return pResult;
		}

		nSize = XHCI_BLOCK_SIZE;
		nAlign = XHCI_BLOCK_ALIGN;
		nBoundary = XHCI_BLOCK_BOUNDARY;
	}

	m_nMemStart += sizeof (TXHCIBlockHeader);

	size_t nAlignMask = nAlign - 1;
	if (m_nMemStart & nAlignMask)
	{
		m_nMemStart += nAlignMask;
		m_nMemStart &= ~nAlignMask;
	}

	size_t nBoundaryMask = nBoundary - 1;
	if (   (m_nMemStart & ~nBoundaryMask)
	    != ((m_nMemStart + nSize-1) & ~nBoundaryMask))
	{
		m_nMemStart += nBoundaryMask;
		m_nMemStart &= ~nBoundaryMask;
	}

	TXHCIBlockHeader *pBlockHeader =
		(TXHCIBlockHeader *) (m_nMemStart - sizeof (TXHCIBlockHeader));

	m_nMemStart += nSize;

	if (m_nMemStart > m_nMemEnd)
	{
		return 0;
	}

	pBlockHeader->nMagic = XHCI_BLOCK_MAGIC;
	pBlockHeader->nSize = (u32) nSize;
	pBlockHeader->nAlign = (u32) nAlign;
	pBlockHeader->nBoundary = (u32) nBoundary;
	pBlockHeader->pNext = 0;

	void *pResult = pBlockHeader->Data;
	assert (((uintptr) pResult & (nAlign-1)) == 0);

	return pResult;
}

void CXHCISharedMemAllocator::Free (void *pBlock)
{
	assert (pBlock != 0);

	TXHCIBlockHeader *pBlockHeader =
		(TXHCIBlockHeader *) ((uintptr) pBlock - sizeof (TXHCIBlockHeader));
	assert (pBlockHeader->nMagic == XHCI_BLOCK_MAGIC);

	if (   pBlockHeader->nSize == XHCI_BLOCK_SIZE
	    && pBlockHeader->nAlign == XHCI_BLOCK_ALIGN
	    && pBlockHeader->nBoundary == XHCI_BLOCK_BOUNDARY)
	{
		pBlockHeader->pNext = m_pFreeList;
		m_pFreeList = pBlockHeader;
	}
	else
	{
		CLogger::Get ()->Write (From, LogWarning,
					"Trying to free shared memory at 0x%lX (size %u, align %u)",
					(uintptr) pBlock, pBlockHeader->nSize, pBlockHeader->nAlign);
	}
}
