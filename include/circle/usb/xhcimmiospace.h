//
// xhcimmiospace.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019-2023  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_xhcimmiospace_h
#define _circle_usb_xhcimmiospace_h

#include <circle/types.h>

class CXHCIMMIOSpace	/// Provides access to the memory-mapped I/O registers of the xHCI controller
{
public:
	CXHCIMMIOSpace (uintptr nBaseAddress);
	~CXHCIMMIOSpace (void);

	u32 cap_read32 (u32 nOffset);
	u32 op_read32 (u32 nOffset);
	u32 pt_read32 (unsigned nPort, u32 nOffset);
	u32 rt_read32 (u32 nOffset);
	u32 rt_read32 (unsigned nInterrupter, u32 nOffset);

	u64 rt_read64 (unsigned nInterrupter, u32 nOffset);

	void op_write32 (u32 nOffset, u32 nValue);
	void db_write32 (unsigned nSlot, u32 nValue);
	void pt_write32 (unsigned nPort, u32 nOffset, u32 nValue);
	void rt_write32 (unsigned nInterrupter, u32 nOffset, u32 nValue);

	void op_write64 (u32 nOffset, u64 nValue);
	void rt_write64 (unsigned nInterrupter, u32 nOffset, u64 nValue);

	boolean op_wait32 (u32 nOffset, u32 nMask, u32 nExpectedValue, unsigned nTimeoutUsecs);

#ifndef NDEBUG
	void DumpStatus (void);
#endif

private:
	u32 cap_read32_raw (u32 nOffset);

private:
	uintptr	m_nBase;	// Capability registers
	uintptr	m_nOpBase;	// Operational registers
	uintptr	m_nDbBase;	// Doorbell registers
	uintptr	m_nRtBase;	// Runtime registers
	uintptr	m_nPtBase;	// Port register set
	uintptr	m_nxECPBase;	// Extended capabilities

	u32 m_nHCxParams[4];	// Capability cache
};

#endif
