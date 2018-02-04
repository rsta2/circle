//
// dwhciframescheduler.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2017  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_dwhciframescheduler_h
#define _circle_usb_dwhciframescheduler_h

#include <circle/sysconfig.h>
#include <circle/types.h>

class CDWHCIFrameScheduler
{
public:
	virtual ~CDWHCIFrameScheduler (void) {}

	virtual void StartSplit (void) = 0;
	virtual boolean CompleteSplit (void) = 0;
	virtual void TransactionComplete (u32 nStatus) = 0;

#ifndef USE_USB_SOF_INTR
	virtual void WaitForFrame (void) = 0;
#else
	virtual u16 GetFrameNumber (void) = 0;

	virtual void PeriodicDelay (u16 usFrameOffset) = 0;
#endif

	virtual boolean IsOddFrame (void) const = 0;
};

#endif
