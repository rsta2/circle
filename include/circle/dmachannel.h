//
// dmachannel.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_dmachannel_h
#define _circle_dmachannel_h

#include <circle/dmacommon.h>
#include <circle/interrupt.h>
#include <circle/machineinfo.h>
#include <circle/macros.h>
#include <circle/types.h>

#if RASPPI >= 4
	#include <circle/dma4channel.h>
#endif

// Not all DMA channels are available to the ARM.
// Channel assignment is made dynamically using the class CMachineInfo.

struct TDMAControlBlock
{
	u32	nTransferInformation;
	u32	nSourceAddress;
	u32	nDestinationAddress;
	u32	nTransferLength;
	u32	n2DModeStride;
	u32	nNextControlBlockAddress;
	u32	nReserved[2];
}
PACKED;

class CDMAChannel
{
public:
	// nChannel must be DMA_CHANNEL_NORMAL, _LITE, _EXTENDED or an explicit channel number
	CDMAChannel (unsigned nChannel, CInterruptSystem *pInterruptSystem = 0);
	~CDMAChannel (void);

	// nBurstLength > 0 increases speed, but may congest the system bus
	void SetupMemCopy (void *pDestination, const void *pSource, size_t nLength,
			   unsigned nBurstLength = 0, boolean bCached = TRUE);
	void SetupIORead (void *pDestination, u32 nIOAddress, size_t nLength, TDREQ DREQ);
	void SetupIOWrite (u32 nIOAddress, const void *pSource, size_t nLength, TDREQ DREQ);

	// copy nBlockCount blocks of nBlockLength size and skip nBlockStride bytes after
	// each block on destination, source is continuous, destination cache is not touched
	// nBurstLength > 0 increases speed, but may congest the system bus
	// (this method is not supported with DMA_CHANNEL_LITE)
	void SetupMemCopy2D (void *pDestination, const void *pSource,
			     size_t nBlockLength, unsigned nBlockCount, size_t nBlockStride,
			     unsigned nBurstLength = 0);

	void SetCompletionRoutine (TDMACompletionRoutine *pRoutine, void *pParam);

	void Start (void);
	boolean Wait (void);		// for synchronous call without completion routine
	boolean GetStatus (void);

private:
	void InterruptHandler (void);
	static void InterruptStub (void *pParam);

private:
	unsigned m_nChannel;

	u8 *m_pControlBlockBuffer;
	TDMAControlBlock *m_pControlBlock;

	CInterruptSystem *m_pInterruptSystem;
	boolean m_bIRQConnected;

	TDMACompletionRoutine *m_pCompletionRoutine;
	void *m_pCompletionParam;

	boolean m_bStatus;

	uintptr m_nDestinationAddress;
	size_t m_nBufferLength;

#if RASPPI >= 4
	CDMA4Channel *m_pDMA4Channel;
#endif
};

#endif
