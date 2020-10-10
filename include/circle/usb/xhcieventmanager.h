//
// xhcieventmanager.h
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
#ifndef _circle_usb_xhcieventmanager_h
#define _circle_usb_xhcieventmanager_h

#include <circle/usb/xhcimmiospace.h>
#include <circle/usb/xhciring.h>
#include <circle/usb/xhci.h>
#include <circle/types.h>

class CXHCIDevice;

class CXHCIEventManager		/// xHC event handling for the xHCI driver
{
public:
	CXHCIEventManager (CXHCIDevice *pXHCIDevice);
	~CXHCIEventManager (void);

	boolean IsValid (void);

	// returns next event dequeue TRB or 0 if event ring is empty
	TXHCITRB *HandleEvents (void);

#ifndef NDEBUG
	void DumpStatus (void);
#endif

private:
	CXHCIDevice	*m_pXHCIDevice;
	CXHCIMMIOSpace	*m_pMMIO;
	CXHCIRing	 m_EventRing;
	TXHCIERSTEntry	*m_pERST;
};

#endif
