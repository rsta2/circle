//
// xhcicommandmanager.h
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
#ifndef _circle_usb_xhcicommandmanager_h
#define _circle_usb_xhcicommandmanager_h

#include <circle/usb/xhcimmiospace.h>
#include <circle/usb/xhciring.h>
#include <circle/usb/xhci.h>
#include <circle/types.h>

#define XHCI_CMD_SUCCESS	XHCI_TRB_SUCCESS

class CXHCIDevice;

class CXHCICommandManager	/// Synchronous xHC command execution for the xHCI driver
{
public:
	CXHCICommandManager (CXHCIDevice *pXHCIDevice);
	~CXHCICommandManager (void);

	boolean IsValid (void);

	// commands
	int EnableSlot (u8 *pSlotID);
	int DisableSlot (u8 uchSlotID);
	int AddressDevice (u8 uchSlotID, TXHCIInputContext *pInputContext, boolean bSetAddress);
	int ConfigureEndpoint (u8 uchSlotID, TXHCIInputContext *pInputContext, boolean bDeconfigure);
	int EvaluateContext (u8 uchSlotID, TXHCIInputContext *pInputContext);
	int NoOp (void);

#ifndef NDEBUG
	void DumpStatus (void);
#endif

private:
	int DoCommand (u32 nControl,		// Cycle bit is set automatically
		       u32 nParameter1 = 0, u32 nParameter2 = 0, u32 nStatus = 0,
		       u8 *pSlotID = 0);	// result

	void CommandCompleted (TXHCITRB *pCommandTRB, u8 uchCompletionCode, u8 uchSlotID);
	friend class CXHCIEventManager;

private:
	CXHCIDevice	*m_pXHCIDevice;
	CXHCIMMIOSpace	*m_pMMIO;

	CXHCIRing	 m_CmdRing;

	volatile boolean m_bCommandCompleted;
	TXHCITRB	*m_pCurrentCommandTRB;
	u8	  	 m_uchCompletionCode;
	u8	  	 m_uchSlotID;
};

#endif
