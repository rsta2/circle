//
// rp1.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023  R. Stange <rsta2@o2online.de>
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
#include <circle/rp1.h>
#include <circle/logger.h>
#include <assert.h>

#define RP1_PCIE_SLOT			0
#define RP1_PCIE_FUNC			0

#define RP1_PCI_CLASS_CODE		0x20000

LOGMODULE ("rp1");

boolean CRP1::s_bIsInitialized = FALSE;

CRP1::CRP1 (CInterruptSystem *pInterrupt)
:	m_PCIe (pInterrupt)
{
}

CRP1::~CRP1 (void)
{
}

boolean CRP1::Initialize (void)
{
	assert (!s_bIsInitialized);

	if (!m_PCIe.Initialize ())
	{
		LOGERR ("Cannot initialize PCIe host bridge");

		return FALSE;
	}

	if (!m_PCIe.EnableDevice (RP1_PCI_CLASS_CODE, RP1_PCIE_SLOT, RP1_PCIE_FUNC))
	{
		LOGERR ("Cannot enable RP1 device");
	}

	s_bIsInitialized = TRUE;

	return TRUE;
}

#ifndef NDEBUG

void CRP1::DumpStatus (void)
{
	if (!s_bIsInitialized)
	{
		LOGDBG ("RP1 device is not initialized");

		return;
	}

	m_PCIe.DumpStatus (RP1_PCIE_SLOT, RP1_PCIE_FUNC);
}

#endif
