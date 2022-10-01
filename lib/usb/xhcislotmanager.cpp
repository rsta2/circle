//
// xhcislotmanager.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019-2022  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/xhcislotmanager.h>
#include <circle/usb/xhcidevice.h>
#include <circle/debug.h>
#include <assert.h>

CXHCISlotManager::CXHCISlotManager (CXHCIDevice *pXHCIDevice)
:	m_pXHCIDevice (pXHCIDevice),
	m_pMMIO (pXHCIDevice->GetMMIOSpace ()),
	m_pDCBAA (0)
{
	for (unsigned nSlotIndex = 0; nSlotIndex < XHCI_CONFIG_MAX_SLOTS; nSlotIndex++)
	{
		m_pUSBDevice[nSlotIndex] = 0;
	}

	m_pMMIO->op_write32 (XHCI_REG_OP_CONFIG,   (m_pMMIO->op_read32 (XHCI_REG_OP_CONFIG)
						 & ~XHCI_REG_OP_CONFIG_MAX_SLOTS_EN__MASK)
						 | XHCI_CONFIG_MAX_SLOTS);

	m_pDCBAA = (u64 *) m_pXHCIDevice->AllocateSharedMem (  (XHCI_CONFIG_MAX_SLOTS + 1)
							     * sizeof (u64));
	if (m_pDCBAA == 0)
	{
		return;
	}

	m_pMMIO->op_write64 (XHCI_REG_OP_DCBAAP, XHCI_TO_DMA (m_pDCBAA));
}

CXHCISlotManager::~CXHCISlotManager (void)
{
	for (unsigned nSlotIndex = 0; nSlotIndex < XHCI_CONFIG_MAX_SLOTS; nSlotIndex++)
	{
		m_pUSBDevice[nSlotIndex] = 0;
	}

	if (m_pDCBAA != 0)
	{
		m_pXHCIDevice->FreeSharedMem (m_pDCBAA);

		m_pDCBAA = 0;
	}

	m_pXHCIDevice = 0;
	m_pMMIO = 0;
}

boolean CXHCISlotManager::IsValid (void)
{
	return m_pDCBAA != 0;
}

void CXHCISlotManager::AssignDevice (u8 uchSlotID, CXHCIUSBDevice *pUSBDevice)
{
	assert (XHCI_IS_SLOTID (uchSlotID));

	assert (pUSBDevice != 0);
	assert (m_pUSBDevice[uchSlotID-1] == 0);
	m_pUSBDevice[uchSlotID-1] = pUSBDevice;

	TXHCIDeviceContext *pDeviceContext = pUSBDevice->GetDeviceContext ();
	assert (pDeviceContext != 0);
	assert (m_pDCBAA != 0);
	assert (m_pDCBAA[uchSlotID] == 0);
	m_pDCBAA[uchSlotID] = XHCI_TO_DMA (pDeviceContext);
}

void CXHCISlotManager::FreeSlot (u8 uchSlotID)
{
	assert (XHCI_IS_SLOTID (uchSlotID));
	m_pUSBDevice[uchSlotID-1] = 0;

	assert (m_pDCBAA != 0);
	m_pDCBAA[uchSlotID] = 0;
}

void CXHCISlotManager::AssignScratchpadBufferArray (u64 *pScratchpadBufferArray)
{
	assert (pScratchpadBufferArray != 0);
	m_pDCBAA[0] = XHCI_TO_DMA (pScratchpadBufferArray);
}

void CXHCISlotManager::TransferEvent (u8 uchCompletionCode, u32 nTransferLength,
				      u8 uchSlotID, u8 uchEndpointID)
{
	assert (XHCI_IS_SLOTID (uchSlotID));
	if (m_pUSBDevice[uchSlotID-1] == 0)
	{
		return;
	}

	m_pUSBDevice[uchSlotID-1]->TransferEvent (uchCompletionCode, nTransferLength, uchEndpointID);
}

#ifndef NDEBUG

void CXHCISlotManager::DumpStatus (void)
{
	debug_hexdump (m_pDCBAA, sizeof (u64) * (XHCI_CONFIG_MAX_SLOTS + 1), "xhcidcbaa");
}

#endif
