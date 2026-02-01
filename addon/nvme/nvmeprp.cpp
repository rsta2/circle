//
// nvmeprp.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2025  R. Stange <rsta2@gmx.net>
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
#include <nvme/nvmeprp.h>
#include <nvme/nvmehelper.h>
#include <circle/util.h>
#include <assert.h>

#define PRP_PAGE_SIZE	NVME_PAGE_SIZE		// Page size used by PRP builder

#define PRP_ENTRY_SIZE	8			// Size per PRP entry

CNVMePRP::CNVMePRP(CNVMeSharedMemAllocator *pAllocator)
:	m_pAllocator(pAllocator),
	m_uPrp1(0),
	m_uPrp2(0),
	m_pPrpListVirt(nullptr),
	m_nPrpListPages(0)
{
}

CNVMePRP::~CNVMePRP()
{
	if (m_pPrpListVirt)
	{
		m_pAllocator->Free(m_pPrpListVirt);
		m_pPrpListVirt = nullptr;
		m_nPrpListPages = 0;
		m_uPrp1 = 0;
		m_uPrp2 = 0;
	}
}

bool CNVMePRP::BuildForBuffer(void *pBuffer, size_t ulLength)
{
	if (!pBuffer || ulLength == 0) return false;

	// Buffer is physically contiguous or described by page addresses
	// We'll compute PRP1 as physical address of the first page + offset within page.
	uintptr uBuf = reinterpret_cast<uintptr>(pBuffer);
	u64 uPhysFirst = PhysicalOf(pBuffer);

	m_uPrp1 = uPhysFirst;

	size_t uRemaining = ulLength;
	size_t uOffsetInFirstPage = uBuf & (PRP_PAGE_SIZE - 1);
	size_t uFirstPageRemaining = PRP_PAGE_SIZE - uOffsetInFirstPage;

	if (uRemaining <= uFirstPageRemaining)
	{
		// Data fits in a single PRP (PRP2 == 0)
		m_uPrp2 = 0;

		return true;
	}

	// Else we need PRP2 to point to next page phys address
	// Compute physical address of second page
	uintptr uSecondPageVirt = (uBuf & ~(PRP_PAGE_SIZE - 1)) + PRP_PAGE_SIZE;
	void *pSecondPagePtr = reinterpret_cast<void *>(uSecondPageVirt);
	u64 uPhysSecond = PhysicalOf(pSecondPagePtr);
	m_uPrp2 = uPhysSecond;

	uRemaining -= uFirstPageRemaining;

	if (uRemaining <= PRP_PAGE_SIZE)
	{
		// No PRP list needed; PRP2 points to last page
		return true;
	}

	// Need PRP list pages. Each PRP list page can hold PRP_PAGE_SIZE / PRP_ENTRY_SIZE entries.
	size_t uEntriesPerPage = PRP_PAGE_SIZE / PRP_ENTRY_SIZE;
	size_t uNeededEntries = (uRemaining + PRP_PAGE_SIZE - 1) / PRP_PAGE_SIZE;

	// Allocate one or more pages for PRP list
	size_t uPages = (uNeededEntries + uEntriesPerPage - 1) / uEntriesPerPage;
	void *pList = m_pAllocator->Allocate(uPages * PRP_PAGE_SIZE, PRP_PAGE_SIZE);
	if (!pList)
	{
		return false;
	}

	memset(pList, 0, uPages * PRP_PAGE_SIZE);

	// Fill list with subsequent page physical addresses
	u64 *pEntries = reinterpret_cast<u64 *>(pList);
	uintptr uCurPageVirt = uSecondPageVirt;
	for (size_t i = 0; i < uNeededEntries; ++i)
	{
		// compute physical of current page
		void *pPagePtr = reinterpret_cast<void *>(uCurPageVirt + i * PRP_PAGE_SIZE);
		pEntries[i] = PhysicalOf(pPagePtr);
	}

	m_pPrpListVirt = pList;
	m_nPrpListPages = uPages;

	// PRP2 should point to the PRP list page physical address
	m_uPrp2 = PhysicalOf(pList);

	return true;
}
