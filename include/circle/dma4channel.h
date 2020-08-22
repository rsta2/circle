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
#ifndef _circle_dma4channel_h
#define _circle_dma4channel_h

#if RASPPI >= 4

#include <circle/dmacommon.h>
#include <circle/interrupt.h>
#include <circle/macros.h>
#include <circle/types.h>

// Not all DMA4 channels are available to the ARM.
// Normally DMA4 channel 11 is allocated by the VPU.
// Normally DMA4 channels 12-14 are available to the ARM.

struct TDMA4ControlBlock
{
	u32	nTransferInformation;
	u32	nSourceAddress;
	u32	nSourceInformation;
	u32	nDestinationAddress;
	u32	nDestinationInformation;
	u32	nTransferLength;
	u32	nNextControlBlockAddress;
	u32	nReserved;
}
PACKED;

/// \note Do not explicitly use this class! Use the class CDMAChannel instead
///       with nChannel set to DMA_CHANNEL_EXTENDED!

class CDMA4Channel	/// Platform DMA4 "large address" controller support
{
public:
	// nChannel must be an explicit channel number (11..14)
	CDMA4Channel (unsigned nChannel, CInterruptSystem *pInterruptSystem = 0);
	~CDMA4Channel (void);

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
	TDMA4ControlBlock *m_pControlBlock;

	CInterruptSystem *m_pInterruptSystem;
	boolean m_bIRQConnected;

	TDMACompletionRoutine *m_pCompletionRoutine;
	void *m_pCompletionParam;

	boolean m_bStatus;

	uintptr m_nDestinationAddress;
	size_t m_nBufferLength;
};

#endif

#endif
