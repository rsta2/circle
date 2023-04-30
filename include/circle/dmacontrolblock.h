//
// dmacontrolblock.h
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
#ifndef _circle_dmacontrolblock_h
#define _circle_dmacontrolblock_h

#include <circle/dmacommon.h>
// #include <circle/interrupt.h>
// #include <circle/machineinfo.h>
// #include <circle/macros.h>
// #include <circle/types.h>

// TODO - support RASPPI 4 channels
// #if RASPPI >= 4
// 	#include <circle/dma4channel.h>
// #endif

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

enum TDMADataWidth
{
	SrcWidth32	= 0,
	SrcWidth128	= 1
};


class CDMAControlBlock	/// Platform DMA controller support
{
public:
	/// \param nChannel DMA_CHANNEL_NORMAL, _LITE, _EXTENDED or an explicit channel number
	/// \param pInterruptSystem Pointer to the interrupt system object\n
	///	   (or 0, if SetCompletionRoutine() is not used)
	CDMAControlBlock ();

	~CDMAControlBlock (void);

	void *GetSourceAddress(boolean bAsBusAddress);
	void SetSourceAddress(void *pSource);

	void *GetDestAddress(boolean bAsBusAddress);
	void SetDestAddress(void *pDest);

	size_t GetTransferLength(void);
	void SetTransferLength(size_t nLength);

	TDREQ GetDREQ(void);
	void SetDREQ(TDREQ DREQ);

	boolean GetUseDREQForSrc(void);
	void SetUseDREQForSrc(boolean bUseDREQForSrc);

	boolean GetUseDREQForDest(void);
	void SetUseDREQForDest(boolean bUseDREQForDest);

	unsigned GetBurstLength(void);
	void SetBurstLength(unsigned nBurstLength);

	unsigned GetSrcWidth(void);
	void SetSrcWidth(TDMADataWidth srcWidth);

	boolean GetSrcIncrement(void);
	void SetSrcIncrement(boolean bSrcIncrement);

	unsigned GetDestWidth(void);
	void SetDestWidth(TDMADataWidth srcWidth);

	boolean GetDestIncrement(void);
	void SetDestIncrement(boolean bSrcIncrement);
	
	boolean GetWaitForWriteResponse(void);
	void SetWaitForWriteResponse(boolean bWaitForWriteResponse);
	
	unsigned GetWaitCycles(void);
	void SetWaitCycles(unsigned nWaitCycles);

	CDMAControlBlock *GetChainedControlBlock(void);
	void SetChainedControlBlock(CDMAControlBlock *pControlBlock);
	
	TDMAControlBlock *GetRawControlBlock(void);
private:
	// Private methods

private:
	u8 *m_pControlBlockBuffer;
	TDMAControlBlock *m_pControlBlock;
	TDMAControlBlock *m_pChainedControlBlock;
};

#endif
