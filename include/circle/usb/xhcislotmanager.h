//
// xhcislotmanager.h
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
#ifndef _circle_usb_xhcislotmanager_h
#define _circle_usb_xhcislotmanager_h

#include <circle/usb/xhcimmiospace.h>
#include <circle/usb/xhciusbdevice.h>
#include <circle/usb/xhci.h>
#include <circle/types.h>

class CXHCIDevice;

class CXHCISlotManager		/// Manages the USB device slots of the xHCI controller
{
public:
	CXHCISlotManager (CXHCIDevice *pXHCIDevice);
	~CXHCISlotManager (void);

	boolean IsValid (void);

	void AssignDevice (u8 uchSlotID, CXHCIUSBDevice *pUSBDevice);
	void FreeSlot (u8 uchSlotID);

	void AssignScratchpadBufferArray (u64 *pScratchpadBufferArray);

#ifndef NDEBUG
	void DumpStatus (void);
#endif

private:
	void TransferEvent (u8 uchCompletionCode, u32 nTransferLength,
			    u8 uchSlotID, u8 uchEndpointID);
	friend class CXHCIEventManager;

private:
	CXHCIDevice	*m_pXHCIDevice;
	CXHCIMMIOSpace	*m_pMMIO;

	u64 *m_pDCBAA;

	CXHCIUSBDevice	*m_pUSBDevice[XHCI_CONFIG_MAX_SLOTS];
};

#endif
