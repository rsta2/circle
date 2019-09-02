//
// xhciring.h
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
#ifndef _circle_usb_xhciring_h
#define _circle_usb_xhciring_h

#include <circle/usb/xhci.h>
#include <circle/types.h>

enum TXHCIRingType
{
	XHCIRingTypeTransfer,
	XHCIRingTypeEvent,
	XHCIRingTypeCommand,
	XHCIRingTypeUnknown
};

class CXHCIDevice;

class CXHCIRing		/// Encapsulates a transfer, command or event ring
{
public:
	CXHCIRing (TXHCIRingType Type, unsigned nTRBCount, CXHCIDevice *pAllocator);
	~CXHCIRing (void);

	boolean IsValid (void) const;

	unsigned GetTRBCount (void) const;

	TXHCITRB *GetFirstTRB (void);
	TXHCITRB *GetDequeueTRB (void);		// returns 0 if empty
	TXHCITRB *GetEnqueueTRB (void);		// returns 0 if full

	TXHCITRB *IncrementDequeue (void);	// returns next dequeue TRB
	void IncrementEnqueue (void);

	u32 GetCycleState (void) const;

#ifndef NDEBUG
	void DumpStatus (const char *pFrom = 0);
#endif

private:
	TXHCIRingType	 m_Type;
	unsigned	 m_nTRBCount;
	CXHCIDevice	*m_pAllocator;

	TXHCITRB	*m_pFirstTRB;
	unsigned	 m_nEnqueueIndex;
	unsigned	 m_nDequeueIndex;
	u32		 m_nCycleState;
};

#endif
