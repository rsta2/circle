//
// dwhcixactqueue.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2022  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_dwhcixactqueue_h
#define _circle_usb_dwhcixactqueue_h

#include <circle/usb/dwhcixferstagedata.h>
#include <circle/ptrlistfiq.h>
#include <circle/spinlock.h>
#include <circle/sysconfig.h>
#include <circle/types.h>

#ifdef USE_USB_SOF_INTR

class CUSBDevice;

class CDWHCITransactionQueue		// Queues coming USB transactions (FIFO)
{
public:
	CDWHCITransactionQueue (unsigned nMaxElements,
				unsigned nMaxAccessLevel);	//  IRQ_LEVEL or FIQ_LEVEL
	~CDWHCITransactionQueue (void);

	// remove all entries
	void Flush (void);

	// remove entries for device
	void FlushDevice (CUSBDevice *pUSBDevice);

	// enqueue transaction to be processed at usFrameNumber
	void Enqueue (CDWHCITransferStageData *pStageData, u16 usFrameNumber);

	// dequeue next transaction to be processed at usFrameNumber (or earlier)
	CDWHCITransferStageData *Dequeue (u16 usFrameNumber);

private:
	CPtrListFIQ m_List;

	CSpinLock m_SpinLock;
};

#endif

#endif
