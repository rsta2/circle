//
// dwhciframeschediso.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2022  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_dwhciframeschediso_h
#define _circle_usb_dwhciframeschediso_h

#include <circle/usb/dwhciframescheduler.h>
#include <circle/sysconfig.h>
#include <circle/classallocator.h>
#include <circle/types.h>

class CDWHCIFrameSchedulerIsochronous : public CDWHCIFrameScheduler
{
public:
	CDWHCIFrameSchedulerIsochronous (boolean bIn);
	~CDWHCIFrameSchedulerIsochronous (void);

	void StartSplit (void);
	boolean CompleteSplit (void);
	void TransactionComplete (u32 nStatus);
	
#ifndef USE_USB_SOF_INTR
	void WaitForFrame (void);
#else
	u16 GetFrameNumber (void);

	void PeriodicDelay (u16 usFrameOffset);
#endif

	boolean IsOddFrame (void) const;

private:
	boolean m_bIn;
	unsigned m_nState;
	u16 m_usFrameOffset;
	boolean m_bFrameAlign;		// microframe number is multiple of 8
	u16 m_usNextFrame;
	unsigned m_nRetries;

	DECLARE_CLASS_ALLOCATOR
};

#endif
