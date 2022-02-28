//
// dmachannel.h
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

/// \note Not all DMA channels are available to the ARM.\n
///	  Channel assignment is normally made dynamically using the class CMachineInfo.

/// \note Each DMA transfer is initiated using a Setup*() method, optionally
///	  SetCompletionRoutine() and Start(), and is completed using Wait()
///	  or GetStatus() from the completion routine.

class CDMAChannel	/// Platform DMA controller support
{
public:
	/// \param nChannel DMA_CHANNEL_NORMAL, _LITE, _EXTENDED or an explicit channel number
	/// \param pInterruptSystem Pointer to the interrupt system object\n
	///	   (or 0, if SetCompletionRoutine() is not used)
	CDMAChannel (unsigned nChannel, CInterruptSystem *pInterruptSystem = 0);

	~CDMAChannel (void);

	/// \brief Prepare a memory copy transfer
	/// \param pDestination Pointer to the destination buffer
	/// \param pSource	Pointer to the source buffer
	/// \param nLength	Number of bytes to be transferred
	/// \param nBurstLength Number of words to be transferred at once (0 = single transfer)
	/// \param bCached	Are the destination and source buffers in cached memory regions
	/// \note nBurstLength > 0 increases speed, but may congest the system bus.
	void SetupMemCopy (void *pDestination, const void *pSource, size_t nLength,
			   unsigned nBurstLength = 0, boolean bCached = TRUE);

	/// \brief Prepare an I/O read transfer
	/// \param pDestination Pointer to the destination buffer
	/// \param nIOAddress	I/O address to be read from (ARM-side or bus address)
	/// \param nLength	Number of bytes to be transferred
	/// \param DREQ		DREQ line for pacing the transfer (see dmacommon.h)
	void SetupIORead (void *pDestination, u32 nIOAddress, size_t nLength, TDREQ DREQ);

	/// \brief Prepare an I/O write transfer
	/// \param nIOAddress	I/O address to be written (ARM-side or bus address)
	/// \param pSource	Pointer to the source buffer
	/// \param nLength	Number of bytes to be transferred
	/// \param DREQ		DREQ line for pacing the transfer (see dmacommon.h)
	void SetupIOWrite (u32 nIOAddress, const void *pSource, size_t nLength, TDREQ DREQ);

	/// \brief Prepare a 2D memory copy transfer (copy a number of blocks with optional stride)
	/// \param pDestination Pointer to the destination buffer
	/// \param pSource	Pointer to the (continuous) source buffer
	/// \param nBlockLength	Length of the blocks to be transferred
	/// \param nBlockCount	Number of blocks to be transferred
	/// \param nBlockStride	Number of bytes to be skipped after each block in destination buffer
	/// \param nBurstLength Number of words to be transferred at once (0 = single transfer)
	/// \note The destination cache is not touched.
	/// \note nBurstLength > 0 increases speed, but may congest the system bus.
	/// \note This method is not supported with DMA_CHANNEL_LITE.
	void SetupMemCopy2D (void *pDestination, const void *pSource,
			     size_t nBlockLength, unsigned nBlockCount, size_t nBlockStride,
			     unsigned nBurstLength = 0);

	/// \brief Set completion routine to be called, when the transfer is finished
	/// \param pRoutine Pointer to the completion routine
	/// \param pParam   User parameter
	void SetCompletionRoutine (TDMACompletionRoutine *pRoutine, void *pParam);

	/// \brief Start the DMA transfer
	void Start (void);

	/// \brief Wait for the completion of the DMA transfer
	/// \return Has the transfer been successful?
	/// \note This is for synchronous calls without completion routine.
	boolean Wait (void);

	/// \brief Get status of the DMA transfer, to be called in the completion routine
	/// \return Has the transfer been successful?
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
