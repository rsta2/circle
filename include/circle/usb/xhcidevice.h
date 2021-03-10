//
// xhcidevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019-2021  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_xhcidevice_h
#define _circle_usb_xhcidevice_h

#include <circle/usb/usbhostcontroller.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/usb/usbrequest.h>
#include <circle/bcmpciehostbridge.h>
#include <circle/usb/xhcisharedmemallocator.h>
#include <circle/usb/xhcimmiospace.h>
#include <circle/usb/xhcislotmanager.h>
#include <circle/usb/xhcieventmanager.h>
#include <circle/usb/xhcicommandmanager.h>
#include <circle/usb/xhciroothub.h>
#include <circle/usb/xhci.h>
#include <circle/sysconfig.h>
#include <circle/types.h>

class CXHCIDevice : public CUSBHostController	/// USB host controller interface (xHCI) driver
{
public:
	CXHCIDevice (CInterruptSystem *pInterruptSystem, CTimer *pTimer,
		     boolean bPlugAndPlay = FALSE);
	~CXHCIDevice (void);

	boolean Initialize (boolean bScanDevices = TRUE);

	void ReScanDevices (void);

	boolean SubmitBlockingRequest (CUSBRequest *pURB, unsigned nTimeoutMs = USB_TIMEOUT_NONE);
	boolean SubmitAsyncRequest (CUSBRequest *pURB, unsigned nTimeoutMs = USB_TIMEOUT_NONE);

public:
	CXHCIMMIOSpace *GetMMIOSpace (void);
	CXHCISlotManager *GetSlotManager (void);
	CXHCICommandManager *GetCommandManager (void);
	CXHCIRootHub *GetRootHub (void);

	// returned memory block has been set to zero
	void *AllocateSharedMem (size_t nSize, size_t nAlign = 64,
				 size_t nBoundary = XHCI_PAGE_SIZE);
	void FreeSharedMem (void *pBlock);

#ifndef NDEBUG
	void DumpStatus (void);
#endif

private:
	void InterruptHandler (void);
	static void InterruptStub (void *pParam);

	boolean HWReset (void);

private:
	CInterruptSystem *m_pInterruptSystem;
	boolean m_bInterruptConnected;

#ifndef USE_XHCI_INTERNAL
	CBcmPCIeHostBridge m_PCIeHostBridge;
#endif

	CXHCISharedMemAllocator m_SharedMemAllocator;

	CXHCIMMIOSpace *m_pMMIO;

	CXHCISlotManager    *m_pSlotManager;
	CXHCIEventManager   *m_pEventManager;
	CXHCICommandManager *m_pCommandManager;

	void *m_pScratchpadBuffers;
	u64  *m_pScratchpadBufferArray;

	CXHCIRootHub *m_pRootHub;

	boolean m_bShutdown;
};

#endif
