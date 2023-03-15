//
// xhcimmiospace.cpp
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
#include <circle/usb/xhcimmiospace.h>
#include <circle/usb/xhci.h>
#include <circle/memio.h>
#include <circle/timer.h>
#include <circle/debug.h>
#include <assert.h>

CXHCIMMIOSpace::CXHCIMMIOSpace (uintptr nBaseAddress)
:	m_nBase (nBaseAddress)
{
	m_nOpBase = m_nBase + read8 (m_nBase + XHCI_REG_CAP_CAPLENGTH);
	m_nDbBase = m_nBase + (cap_read32_raw (XHCI_REG_CAP_DBOFF) & XHCI_REG_CAP_DBOFF__MASK);
	m_nRtBase = m_nBase + (cap_read32_raw (XHCI_REG_CAP_RTSOFF) & XHCI_REG_CAP_RTSOFF__MASK);
	m_nPtBase = m_nOpBase + XHCI_REG_OP_PORTS_BASE;

	m_nHCxParams[0] = cap_read32_raw (XHCI_REG_CAP_HCSPARAMS1);
	m_nHCxParams[1] = cap_read32_raw (XHCI_REG_CAP_HCSPARAMS2);
	m_nHCxParams[2] = cap_read32_raw (XHCI_REG_CAP_HCSPARAMS3);
	m_nHCxParams[3] = cap_read32_raw (XHCI_REG_CAP_HCCPARAMS);

	m_nxECPBase = m_nBase + (  (cap_read32 (XHCI_REG_CAP_HCCPARAMS)
				 & XHCI_REG_CAP_HCCPARAMS1_XECP__MASK)
				>> XHCI_REG_CAP_HCCPARAMS1_XECP__SHIFT
				<< 2);
}

CXHCIMMIOSpace::~CXHCIMMIOSpace (void)
{
	m_nBase = 0;
}

u32 CXHCIMMIOSpace::cap_read32 (u32 nOffset)
{
	assert (m_nBase != 0);

	nOffset -= XHCI_REG_CAP_HCSPARAMS1;
	nOffset /= 4;

	assert (nOffset < 4);
	return m_nHCxParams[nOffset];
}

u32 CXHCIMMIOSpace::op_read32 (u32 nOffset)
{
	assert (m_nBase != 0);

	return read32 (m_nOpBase + nOffset);
}

u32 CXHCIMMIOSpace::pt_read32 (unsigned nPort, u32 nOffset)
{
	assert (m_nBase != 0);

	return read32 (m_nPtBase + nPort * XHCI_REG_OP_PORTS_PORT__SIZE + nOffset);
}

u32 CXHCIMMIOSpace::rt_read32 (u32 nOffset)
{
	assert (m_nBase != 0);

	return read32 (m_nRtBase + nOffset);
}

u32 CXHCIMMIOSpace::rt_read32 (unsigned nInterrupter, u32 nOffset)
{
	assert (m_nBase != 0);

	return read32 (m_nRtBase + XHCI_REG_RT_IR0 + nInterrupter * XHCI_REG_RT_IR__SIZE + nOffset);
}

u64 CXHCIMMIOSpace::rt_read64 (unsigned nInterrupter, u32 nOffset)
{
	assert (m_nBase != 0);

	uintptr nAddress =
		m_nRtBase + XHCI_REG_RT_IR0 + nInterrupter * XHCI_REG_RT_IR__SIZE + nOffset;

	u64 nValue = read32 (nAddress);
	nValue |= (u64) read32 (nAddress+4) << 32;

	return nValue;
}

void CXHCIMMIOSpace::op_write32 (u32 nOffset, u32 nValue)
{
	assert (m_nBase != 0);

	write32 (m_nOpBase + nOffset, nValue);
}

void CXHCIMMIOSpace::db_write32 (unsigned nSlot, u32 nValue)
{
	assert (m_nBase != 0);

	write32 (m_nDbBase + nSlot * 4, nValue);
}

void CXHCIMMIOSpace::pt_write32 (unsigned nPort, u32 nOffset, u32 nValue)
{
	assert (m_nBase != 0);

	write32 (m_nPtBase + nPort * XHCI_REG_OP_PORTS_PORT__SIZE + nOffset, nValue);
}

void CXHCIMMIOSpace::rt_write32 (unsigned nInterrupter, u32 nOffset, u32 nValue)
{
	assert (m_nBase != 0);

	write32 (m_nRtBase + XHCI_REG_RT_IR0 + nInterrupter * XHCI_REG_RT_IR__SIZE + nOffset, nValue);
}

void CXHCIMMIOSpace::op_write64 (u32 nOffset, u64 nValue)
{
	op_write32 (nOffset, (u32) nValue);
	op_write32 (nOffset+4, (u32) (nValue >> 32));
}

void CXHCIMMIOSpace::rt_write64 (unsigned nInterrupter, u32 nOffset, u64 nValue)
{
	assert (m_nBase != 0);

	uintptr nAddress =
		m_nRtBase + XHCI_REG_RT_IR0 + nInterrupter * XHCI_REG_RT_IR__SIZE + nOffset;

	write32 (nAddress, (u32) nValue);
	write32 (nAddress+4, (u32) (nValue >> 32));
}

boolean CXHCIMMIOSpace::op_wait32 (u32 nOffset, u32 nMask, u32 nExpectedValue,
				   unsigned nTimeoutUsecs)
{
	unsigned nTimeoutTicks = nTimeoutUsecs * (CLOCKHZ / 1000000);
	unsigned nStartTicks = CTimer::Get ()->GetClockTicks ();

	do
	{
		if ((op_read32 (nOffset) & nMask) == nExpectedValue)
		{
			return TRUE;
		}
	}
	while (CTimer::Get ()->GetClockTicks () - nStartTicks < nTimeoutTicks);

	return FALSE;
}

#ifndef NDEBUG

void CXHCIMMIOSpace::DumpStatus (void)
{
	assert (m_nBase != 0);

	debug_hexdump ((void *) m_nBase,   0x1C, "xhcicap");
	debug_hexdump ((void *) m_nOpBase, 0x3C, "xhciop");
	debug_hexdump ((void *) (m_nRtBase + XHCI_REG_RT_IR0), XHCI_REG_RT_IR__SIZE, "xhciir0");
}

#endif

u32 CXHCIMMIOSpace::cap_read32_raw (u32 nOffset)
{
	assert (m_nBase != 0);

	return read32 (m_nBase + nOffset);
}
