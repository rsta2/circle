//
// dmasoundbuffers.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2021  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_dmasoundbuffers_h
#define _circle_dmasoundbuffers_h

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

	typedef unsigned TChunkCompletedHandler (boolean    bStatus,
						 u32	   *pBuffer,
						 unsigned   nChunkSize,
						 void	   *pParam);

public:
	CDMASoundBuffers (u32		    nIOAddress,
			  TDREQ		    DREQ,
			  unsigned	    nChunkSize,
			  CInterruptSystem *pInterruptSystem);

	~CDMASoundBuffers (void);

	boolean Start (TChunkCompletedHandler *pHandler, void *pParam = 0);

	void Cancel (void);

	boolean IsActive (void) const;

private:
	boolean GetNextChunk (boolean bFirstCall);

	void InterruptHandler (void);
	static void InterruptStub (void *pParam);

	boolean SetupDMAControlBlock (unsigned nID);

private:
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
