//
// xhciroothub.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019-2020  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/xhciroothub.h>
#include <circle/usb/xhcidevice.h>
#include <circle/logger.h>
#include <circle/koptions.h>
#include <circle/timer.h>
#include <assert.h>

CXHCIRootHub::CXHCIRootHub (unsigned nPorts, CXHCIDevice *pXHCIDevice)
:	m_nPorts (nPorts),
	m_pXHCIDevice (pXHCIDevice)
{
	for (unsigned nPort = 0; nPort < m_nPorts; nPort++)
	{
		m_pRootPort[nPort] = new CXHCIRootPort (nPort+1, m_pXHCIDevice);
		assert (m_pRootPort[nPort] != 0);
	}
}

CXHCIRootHub::~CXHCIRootHub (void)
{
	for (unsigned nPort = 0; nPort < m_nPorts; nPort++)
	{
		delete m_pRootPort[nPort];
		m_pRootPort[nPort] = 0;
	}
}

boolean CXHCIRootHub::Initialize (void)
{
	unsigned nMsDelay = 300;		// best guess standard delay
	CKernelOptions *pOptions = CKernelOptions::Get ();
	if (pOptions != 0)
	{
		unsigned nUSBPowerDelay = pOptions->GetUSBPowerDelay ();
		if (nUSBPowerDelay != 0)
		{
			nMsDelay = nUSBPowerDelay;
		}
	}

	CTimer::Get ()->MsDelay (nMsDelay);	// give root ports some time to settle

	for (unsigned nPort = 0; nPort < m_nPorts; nPort++)
	{
		assert (m_pRootPort[nPort] != 0);
		m_pRootPort[nPort]->Initialize ();
	}

	for (unsigned nPort = 0; nPort < m_nPorts; nPort++)
	{
		assert (m_pRootPort[nPort] != 0);
		m_pRootPort[nPort]->Configure ();
	}

	return TRUE;
}

boolean CXHCIRootHub::ReScanDevices (void)
{
	boolean bResult = FALSE;

	for (unsigned nPort = 0; nPort < m_nPorts; nPort++)
	{
		assert (m_pRootPort[nPort] != 0);
		if (m_pRootPort[nPort]->ReScanDevices ())
		{
			bResult = TRUE;
		}
	}

	return bResult;
}

void CXHCIRootHub::StatusChanged (u8 uchPortID)
{
	unsigned nPort = uchPortID-1;
	assert (nPort < m_nPorts);
	assert (m_pRootPort[nPort] != 0);

	m_pRootPort[nPort]->StatusChanged ();
}

#ifndef NDEBUG

void CXHCIRootHub::DumpStatus (void)
{
	for (unsigned nPort = 0; nPort < m_nPorts; nPort++)
	{
		m_pRootPort[nPort]->DumpStatus ();
	}
}

#endif
