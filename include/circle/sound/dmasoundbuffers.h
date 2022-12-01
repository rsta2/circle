//
// dmasoundbuffers.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2021-2022  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_sound_dmasoundbuffers_h
#define _circle_sound_dmasoundbuffers_h

#include <circle/dmachannel.h>
#include <circle/interrupt.h>
#include <circle/spinlock.h>
#include <circle/types.h>

class CDMASoundBuffers	/// Concatenated DMA buffers to be used by sound device drivers
{
public:
	enum TState
	{
		StateCreated,
		StateIdle,
		StateRunning,
		StateCancelled,
		StateTerminating,
		StateFailed,
		StateUnknown
	};

	/// \param bStatus FALSE if an error occurred
	/// \param pBuffer Pointer to the buffer, which provides (RX) or receives the samples (TX)
	/// \param nChunkSize Size of the buffer in number of 32-bit words
	/// \param pParam User parameter, which was handed over to Start()
	typedef unsigned TChunkCompletedHandler (boolean    bStatus,
						 u32	   *pBuffer,
						 unsigned   nChunkSize,
						 void	   *pParam);

public:
	/// \param bDirectionOut TRUE if these buffers are used for sound output
	/// \param nIOAddress ARM memory address of the target data device register
	/// \param DREQ DREQ number to be used to pace the transfer
	/// \param nChunkSize Size of a chunk, DMA transferred at once, in number of 32-bit words
	/// \param pInterruptSystem Pointer to the interrupt system
	CDMASoundBuffers (boolean	    bDirectionOut,
			  u32		    nIOAddress,
			  TDREQ		    DREQ,
			  unsigned	    nChunkSize,
			  CInterruptSystem *pInterruptSystem);

	~CDMASoundBuffers (void);

	/// \brief Start DMA operation
	/// \param pHandler Callback handler, which gets called, when one chunk was completed
	/// \param pParam User parameter, which will be handed over to the handler
	boolean Start (TChunkCompletedHandler *pHandler, void *pParam = 0);

	/// \brief Cancel DMA operation
	/// \note Does not stop immediately, use IsActive() to check if operation is still running
	void Cancel (void);

	/// \return Is DMA operation running?
	boolean IsActive (void) const;

private:
	boolean GetNextChunk (boolean bFirstCall);
	boolean PutChunk (void);

	void InterruptHandler (void);
	static void InterruptStub (void *pParam);

	boolean SetupDMAControlBlock (unsigned nID);

private:
	boolean m_bDirectionOut;
	u32 m_nIOAddress;
	TDREQ m_DREQ;
	unsigned m_nChunkSize;
	CInterruptSystem *m_pInterruptSystem;

	TChunkCompletedHandler *m_pHandler;
	void *m_pParam;

	boolean m_bIRQConnected;
	volatile TState m_State;

	unsigned m_nDMAChannel;
	u32 *m_pDMABuffer[2];
	TDMAControlBlock *m_pControlBlock[2];

	unsigned m_nNextBuffer;		// 0 or 1

	CSpinLock m_SpinLock;
};

#endif
