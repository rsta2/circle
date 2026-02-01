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
#ifndef _nvme_nvmeprp_h
#define _nvme_nvmeprp_h

#include <nvme/nvmesharedmemallocator.h>
#include <circle/types.h>

class CNVMePRP	// Helper to build PRP1 / PRP2 and optional PRP list pages for a given buffer
{
public:
	CNVMePRP(CNVMeSharedMemAllocator *pAllocator);
	~CNVMePRP(void);

	// Build PRP descriptors for given buffer and length (bytes)
	bool BuildForBuffer(void *pBuffer, size_t ulLength);

	u64 Prp1(void) const	{ return m_uPrp1; }
	u64 Prp2(void) const	{ return m_uPrp2; }

private:
	CNVMeSharedMemAllocator *m_pAllocator;

	u64 m_uPrp1;
	u64 m_uPrp2;

	void *m_pPrpListVirt;		// virtual pointer of PRP list page (if allocated)
	unsigned m_nPrpListPages;	// number of pages used for PRP list
};

#endif
