//
// dwhcicompletionqueue.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2022  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_dwhcicompletionqueue_h
#define _circle_usb_dwhcicompletionqueue_h

#include <circle/usb/usbrequest.h>
#include <circle/ptrlistfiq.h>
#include <circle/sysconfig.h>
#include <circle/spinlock.h>
#include <circle/types.h>

#ifdef USE_USB_FIQ

class CDWHCICompletionQueue		// Queues USB requests ready for completion (FIFO)
{
public:
	CDWHCICompletionQueue (unsigned nMaxElements);
	~CDWHCICompletionQueue (void);

	boolean IsEmpty (void);

	void Enqueue (CUSBRequest *pURB);

	CUSBRequest *Dequeue (void);

private:
	CPtrListFIQ m_List;

	CSpinLock m_SpinLock;
};

#endif

#endif
