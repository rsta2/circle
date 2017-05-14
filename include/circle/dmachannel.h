//
// dmachannel.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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

#include <circle/interrupt.h>
#include <circle/macros.h>
#include <circle/types.h>

// Not all DMA channels are available to the ARM.
// Normally channels 0, 2, 4 and 5 are available.

// channel assignment
#define DMA_CHANNEL_PWM		0

#define DMA_CHANNEL_SPI_TX	0
#define DMA_CHANNEL_SPI_RX	2

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

enum TDREQ
{
	DREQSourceNone	 = 0,
	DREQSourcePCMTX	 = 2,
	DREQSourcePCMRX	 = 3,
	DREQSourcePWM	 = 5,
	DREQSourceSPITX	 = 6,
	DREQSourceSPIRX	 = 7,
	DREQSourceEMMC	 = 11,
	DREQSourceUARTTX = 12,
	DREQSourceUARTRX = 14
};

typedef void TDMACompletionRoutine (unsigned nChannel, boolean bStatus, void *pParam);

class CDMAChannel
{
public:
	CDMAChannel (unsigned nChannel, CInterruptSystem *pInterruptSystem = 0);
	~CDMAChannel (void);

	void SetupMemCopy (void *pDestination, const void *pSource, size_t nLength);
	void SetupIORead (void *pDestination, u32 nIOAddress, size_t nLength, TDREQ DREQ);
	void SetupIOWrite (u32 nIOAddress, const void *pSource, size_t nLength, TDREQ DREQ);

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

	u32 m_nDestinationAddress;
	u32 m_nBufferLength;
};

#endif
