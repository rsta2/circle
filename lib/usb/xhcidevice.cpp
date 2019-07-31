//
// xhcidevice.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/xhcidevice.h>
#include <circle/bcm2711.h>
#include <circle/memio.h>
#include <circle/logger.h>
#include <assert.h>

#define XHCI_PCIE_SLOT		0
#define XHCI_PCIE_FUNC		0

#define XHCI_PCI_CLASS_CODE	0xC0330

static const char FromXHCI[] = "xhci";

CXHCIDevice::CXHCIDevice (CInterruptSystem *pInterruptSystem, CTimer *pTimer)
:	m_PCIeHostBridge (pInterruptSystem)
{
}

CXHCIDevice::~CXHCIDevice (void)
{
}

boolean CXHCIDevice::Initialize (void)
{
	// PCIe init
	if (!m_PCIeHostBridge.Initialize ())
	{
		CLogger::Get ()->Write (FromXHCI, LogError, "Cannot initialize PCIe host bridge");

		return FALSE;
	}

	if (!m_PCIeHostBridge.ConnectMSI (InterruptStub, this))
	{
		CLogger::Get ()->Write (FromXHCI, LogError, "Cannot connect MSI");

		return FALSE;
	}

	if (!m_PCIeHostBridge.EnableDevice (XHCI_PCI_CLASS_CODE, XHCI_PCIE_SLOT, XHCI_PCIE_FUNC))
	{
		CLogger::Get ()->Write (FromXHCI, LogError, "Cannot enable xHCI device");

		return FALSE;
	}

	// version check
	u16 usVersion = read16 (ARM_XHCI_BASE + 0x02);
	if (usVersion != 0x100)
	{
		CLogger::Get ()->Write (FromXHCI, LogError, "Unsupported xHCI version (%X)",
					(unsigned) usVersion);

		return FALSE;
	}

	return TRUE;
}

void CXHCIDevice::ReScanDevices (void)
{
}

boolean CXHCIDevice::SubmitBlockingRequest (CUSBRequest *pURB, unsigned nTimeoutMs)
{
	return FALSE;
}

boolean CXHCIDevice::SubmitAsyncRequest (CUSBRequest *pURB, unsigned nTimeoutMs)
{
	return FALSE;
}

void CXHCIDevice::InterruptHandler (unsigned nVector)
{
	CLogger::Get ()->Write (FromXHCI, LogDebug, "MSI%u", nVector);
}

void CXHCIDevice::InterruptStub (unsigned nVector, void *pParam)
{
	CXHCIDevice *pThis = (CXHCIDevice *) pParam;
	assert (pThis != 0);

	pThis->InterruptHandler (nVector);
}
