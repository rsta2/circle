//
// dwhcidevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2015  R. Stange <rsta2@o2online.de>
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
#ifndef _dwhcidevice_h
#define _dwhcidevice_h

#include <circle/usb/usbhostcontroller.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/usb/usbendpoint.h>
#include <circle/usb/usbrequest.h>
#include <circle/usb/dwhcirootport.h>
#include <circle/usb/dwhcixferstagedata.h>
#include <circle/usb/dwhciregister.h>
#include <circle/usb/dwhci.h>
#include <circle/usb/usb.h>
#include <circle/spinlock.h>
#include <circle/types.h>

#define DWHCI_WAIT_BLOCKS	DWHCI_MAX_CHANNELS

class CDWHCIDevice : public CUSBHostController
{
public:
	CDWHCIDevice (CInterruptSystem *pInterruptSystem, CTimer *pTimer);
	~CDWHCIDevice (void);

	boolean Initialize (void);

	boolean SubmitBlockingRequest (CUSBRequest *pURB);
	boolean SubmitAsyncRequest (CUSBRequest *pURB);

private:
	TUSBSpeed GetPortSpeed (void);
	boolean OvercurrentDetected (void);
	void DisableRootPort (void);
	friend class CDWHCIRootPort;

private:
	boolean InitCore (void);
	boolean InitHost (void);
	boolean EnableRootPort (void);

	boolean PowerOn (void);
	boolean Reset (void);

	void EnableGlobalInterrupts (void);
	void EnableCommonInterrupts (void);
	void EnableHostInterrupts (void);
	void EnableChannelInterrupt (unsigned nChannel);
	void DisableChannelInterrupt (unsigned nChannel);

	void FlushTxFIFO (unsigned nFIFO);
	void FlushRxFIFO (void);

	boolean TransferStage (CUSBRequest *pURB, boolean bIn, boolean bStatusStage);
	static void CompletionRoutine (CUSBRequest *pURB, void *pParam, void *pContext);

	boolean TransferStageAsync (CUSBRequest *pURB, boolean bIn, boolean bStatusStage);

	void StartTransaction (CDWHCITransferStageData *pStageData);
	void StartChannel (CDWHCITransferStageData *pStageData);

	void ChannelInterruptHandler (unsigned nChannel);
	void InterruptHandler (void);
	static void InterruptStub (void *pParam);

	void TimerHandler (CDWHCITransferStageData *pStageData);
	static void TimerStub (unsigned hTimer, void *pParam, void *pContext);

	unsigned AllocateChannel (void);
	void FreeChannel (unsigned nChannel);

	unsigned AllocateWaitBlock (void);
	void FreeWaitBlock (unsigned nWaitBlock);

	boolean WaitForBit (CDWHCIRegister *pRegister,
			    u32		    nMask,
			    boolean	    bWaitUntilSet,
			    unsigned	    nMsTimeout);
	
#ifndef NDEBUG
	void DumpRegister (const char *pName, u32 nAddress);
	void DumpStatus (unsigned nChannel = 0);
#endif
	
private:
	CInterruptSystem *m_pInterruptSystem;
	CTimer *m_pTimer;

	unsigned m_nChannels;
	volatile unsigned m_nChannelAllocated;		// one bit per channel, set if allocated
	CSpinLock m_ChannelSpinLock;

	CDWHCITransferStageData *m_pStageData[DWHCI_MAX_CHANNELS];

	CSpinLock m_IntMaskSpinLock;

	volatile boolean m_bWaiting[DWHCI_WAIT_BLOCKS];
	volatile unsigned m_nWaitBlockAllocated;	// one bit per wait block, set if allocated
	CSpinLock m_WaitBlockSpinLock;

	CDWHCIRootPort m_RootPort;
};

#endif
