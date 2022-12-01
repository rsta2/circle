//
// dmasoundbuffers.cpp
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
#include <circle/sound/dmasoundbuffers.h>
#include <circle/machineinfo.h>
#include <circle/synchronize.h>
#include <circle/bcm2835int.h>
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/timer.h>
#include <circle/util.h>
#include <circle/new.h>
#include <assert.h>

CDMASoundBuffers::CDMASoundBuffers (boolean	      bDirectionOut,
				    u32		      nIOAddress,
				    TDREQ	      DREQ,
				    unsigned	      nChunkSize,
				    CInterruptSystem *pInterruptSystem)
:	m_bDirectionOut {bDirectionOut},
	m_nIOAddress {nIOAddress},
	m_DREQ {DREQ},
	m_nChunkSize {nChunkSize},
	m_pInterruptSystem {pInterruptSystem},
	m_pHandler {0},
	m_bIRQConnected {FALSE},
	m_State {StateCreated},
	m_nDMAChannel {DMA_CHANNEL_MAX+1},
	m_pDMABuffer {nullptr, nullptr},
	m_pControlBlock {nullptr, nullptr}
{
}

CDMASoundBuffers::~CDMASoundBuffers (void)
{
	assert (   m_State != StateRunning
		&& m_State != StateCancelled
		&& m_State != StateTerminating);

	if (m_nDMAChannel <= DMA_CHANNEL_MAX)
	{
		if (m_bIRQConnected)
		{
			assert (m_pInterruptSystem != 0);
			m_pInterruptSystem->DisconnectIRQ (ARM_IRQ_DMA0+m_nDMAChannel);
		}

		PeripheralEntry ();

		write32 (ARM_DMACHAN_CS (m_nDMAChannel), CS_RESET);
		while (read32 (ARM_DMACHAN_CS (m_nDMAChannel)) & CS_RESET)
		{
			// do nothing
		}

		write32 (ARM_DMA_ENABLE, read32 (ARM_DMA_ENABLE) & ~(1 << m_nDMAChannel));

		PeripheralExit ();

		CMachineInfo::Get ()->FreeDMAChannel (m_nDMAChannel);

		m_nDMAChannel = DMA_CHANNEL_MAX+1;
	}

	delete m_pControlBlock[0];
	delete m_pControlBlock[1];
	delete [] m_pDMABuffer[0];
	delete [] m_pDMABuffer[1];
}

boolean CDMASoundBuffers::Start (TChunkCompletedHandler *pHandler, void *pParam)
{
	if (m_State == StateCreated)
	{
		assert (m_nChunkSize * sizeof (u32) <= TXFR_LEN_MAX_LITE);
		m_nDMAChannel = CMachineInfo::Get ()->AllocateDMAChannel (DMA_CHANNEL_LITE);
		if (m_nDMAChannel > DMA_CHANNEL_MAX)
		{
			m_State = StateFailed;

			return FALSE;
		}

		// setup and concatenate DMA buffers and control blocks
		if (   !SetupDMAControlBlock (0)
		    || !SetupDMAControlBlock (1))
		{
			m_State = StateFailed;

			return FALSE;
		}

		m_pControlBlock[0]->nNextControlBlockAddress =
			BUS_ADDRESS ((uintptr) m_pControlBlock[1]);
		m_pControlBlock[1]->nNextControlBlockAddress =
			BUS_ADDRESS ((uintptr) m_pControlBlock[0]);

		CleanAndInvalidateDataCacheRange ((uintptr) m_pControlBlock[0],
						  sizeof (TDMAControlBlock));
		CleanAndInvalidateDataCacheRange ((uintptr) m_pControlBlock[1],
						  sizeof (TDMAControlBlock));

		// enable and reset DMA channel
		PeripheralEntry ();

		assert (m_nDMAChannel <= DMA_CHANNEL_MAX);
		write32 (ARM_DMA_ENABLE, read32 (ARM_DMA_ENABLE) | (1 << m_nDMAChannel));
		CTimer::SimpleusDelay (1000);

		write32 (ARM_DMACHAN_CS (m_nDMAChannel), CS_RESET);
		while (read32 (ARM_DMACHAN_CS (m_nDMAChannel)) & CS_RESET)
		{
			// do nothing
		}

		PeripheralExit ();

		// connect IRQ
		assert (!m_bIRQConnected);
		assert (m_pInterruptSystem != 0);
		assert (m_nDMAChannel <= DMA_CHANNEL_MAX);
		m_pInterruptSystem->ConnectIRQ (ARM_IRQ_DMA0+m_nDMAChannel, InterruptStub, this);
		m_bIRQConnected = TRUE;

		m_State = StateIdle;
	}

	assert (m_State == StateIdle);

	assert (pHandler != 0);
	m_pHandler = pHandler;
	m_pParam = pParam;

	// fill buffer 0
	m_nNextBuffer = 0;

	if (   m_bDirectionOut
	    && !GetNextChunk (TRUE))
	{
		return FALSE;
	}

	m_State = StateRunning;

	// start DMA
	PeripheralEntry ();

	assert (!(read32 (ARM_DMACHAN_CS (m_nDMAChannel)) & CS_INT));
	assert (!(read32 (ARM_DMA_INT_STATUS) & (1 << m_nDMAChannel)));

	assert (m_pControlBlock[0] != 0);
	write32 (ARM_DMACHAN_CONBLK_AD (m_nDMAChannel), BUS_ADDRESS ((uintptr) m_pControlBlock[0]));

	write32 (ARM_DMACHAN_CS (m_nDMAChannel),   CS_WAIT_FOR_OUTSTANDING_WRITES
					         | (DEFAULT_PANIC_PRIORITY << CS_PANIC_PRIORITY_SHIFT)
					         | (DEFAULT_PRIORITY << CS_PRIORITY_SHIFT)
					         | CS_ACTIVE);

	PeripheralExit ();

	// fill buffer 1
	if (   m_bDirectionOut
	    && !GetNextChunk (FALSE))
	{
		m_SpinLock.Acquire ();

		if (m_State == StateRunning)
		{
			PeripheralEntry ();
			write32 (ARM_DMACHAN_NEXTCONBK (m_nDMAChannel), 0);
			PeripheralExit ();

			m_State = StateTerminating;
		}

		m_SpinLock.Release ();
	}

	return TRUE;
}

void CDMASoundBuffers::Cancel (void)
{
	m_SpinLock.Acquire ();

	if (m_State == StateRunning)
	{
		m_State = StateCancelled;
	}

	m_SpinLock.Release ();
}

boolean CDMASoundBuffers::IsActive (void) const
{
	TState State = m_State;

	return    State == StateRunning
	       || State == StateCancelled
	       || State == StateTerminating;
}

boolean CDMASoundBuffers::GetNextChunk (boolean bFirstCall)
{
	assert (m_nNextBuffer <= 1);
	assert (m_pDMABuffer[m_nNextBuffer] != 0);

	unsigned nChunkSize;
	if (!bFirstCall)
	{
		assert (m_pHandler != 0);
		nChunkSize = (*m_pHandler) (TRUE, m_pDMABuffer[m_nNextBuffer],
					    m_nChunkSize, m_pParam);
		if (nChunkSize == 0)
		{
			return FALSE;
		}
	}
	else
	{
		nChunkSize = m_nChunkSize;
		memset (m_pDMABuffer[m_nNextBuffer], 0, nChunkSize * sizeof (u32));
	}

	unsigned nTransferLength = nChunkSize * sizeof (u32);
	assert (nTransferLength <= TXFR_LEN_MAX_LITE);

	assert (m_pControlBlock[m_nNextBuffer] != 0);
	m_pControlBlock[m_nNextBuffer]->nTransferLength = nTransferLength;

	CleanAndInvalidateDataCacheRange ((uintptr) m_pDMABuffer[m_nNextBuffer], nTransferLength);
	CleanAndInvalidateDataCacheRange ((uintptr) m_pControlBlock[m_nNextBuffer], sizeof (TDMAControlBlock));

	m_nNextBuffer ^= 1;

	return TRUE;
}

boolean CDMASoundBuffers::PutChunk (void)
{
	assert (m_nNextBuffer <= 1);
	assert (m_pDMABuffer[m_nNextBuffer] != 0);

	// TODO: must not write DMA buffer from handler (read-only)
	CleanAndInvalidateDataCacheRange ((uintptr) m_pDMABuffer[m_nNextBuffer],
					  m_nChunkSize * sizeof (u32));

	assert (m_pHandler != 0);
	(*m_pHandler) (TRUE, m_pDMABuffer[m_nNextBuffer], m_nChunkSize, m_pParam);

	m_nNextBuffer ^= 1;

	return TRUE;
}

void CDMASoundBuffers::InterruptHandler (void)
{
	assert (m_nDMAChannel <= DMA_CHANNEL_MAX);

	PeripheralEntry ();

#ifndef NDEBUG
	u32 nIntStatus = read32 (ARM_DMA_INT_STATUS);
#endif
	u32 nIntMask = 1 << m_nDMAChannel;
	assert (nIntStatus & nIntMask);
	write32 (ARM_DMA_INT_STATUS, nIntMask);

	u32 nCS = read32 (ARM_DMACHAN_CS (m_nDMAChannel));
	assert (nCS & CS_INT);
	write32 (ARM_DMACHAN_CS (m_nDMAChannel), nCS);	// reset CS_INT

	PeripheralExit ();

	if (nCS & CS_ERROR)
	{
		m_State = StateFailed;

		assert (m_pHandler != 0);
		(*m_pHandler) (FALSE, 0, 0, m_pParam);

		return;
	}

	m_SpinLock.Acquire ();

	switch (m_State)
	{
	case StateRunning:
		if (m_bDirectionOut)
		{
			if (GetNextChunk (FALSE))
			{
				break;
			}
		}
		else
		{
			if (PutChunk ())
			{
				break;
			}
		}
		// fall through

	case StateCancelled:
		PeripheralEntry ();
		write32 (ARM_DMACHAN_NEXTCONBK (m_nDMAChannel), 0);
		PeripheralExit ();

		m_State = StateTerminating;
		break;

	case StateTerminating:
		m_State = StateIdle;
		break;

	default:
		assert (0);
		break;
	}

	m_SpinLock.Release ();
}

void CDMASoundBuffers::InterruptStub (void *pParam)
{
	CDMASoundBuffers *pThis = static_cast <CDMASoundBuffers *> (pParam);
	assert (pThis != 0);

	pThis->InterruptHandler ();
}

boolean CDMASoundBuffers::SetupDMAControlBlock (unsigned nID)
{
	assert (nID <= 1);

	assert (m_nChunkSize > 0);
	m_pDMABuffer[nID] = new (HEAP_DMA30) u32[m_nChunkSize];
	if (!m_pDMABuffer[nID])
	{
		return FALSE;
	}

	m_pControlBlock[nID] = new (HEAP_DMA30) TDMAControlBlock;
	if (!m_pControlBlock[nID])
	{
		return FALSE;
	}

	if (m_bDirectionOut)
	{
		m_pControlBlock[nID]->nTransferInformation =   (m_DREQ << TI_PERMAP_SHIFT)
							     | (   DEFAULT_BURST_LENGTH
								 << TI_BURST_LENGTH_SHIFT)
							     | TI_SRC_WIDTH
							     | TI_SRC_INC
							     | TI_DEST_DREQ
							     | TI_WAIT_RESP
							     | TI_INTEN;
		m_pControlBlock[nID]->nSourceAddress = BUS_ADDRESS ((uintptr) m_pDMABuffer[nID]);
		m_pControlBlock[nID]->nDestinationAddress = (m_nIOAddress & 0xFFFFFF) + GPU_IO_BASE;
	}
	else
	{
		m_pControlBlock[nID]->nTransferInformation =   (m_DREQ << TI_PERMAP_SHIFT)
							     | (   DEFAULT_BURST_LENGTH
								 << TI_BURST_LENGTH_SHIFT)
							     | TI_DEST_WIDTH
							     | TI_DEST_INC
							     | TI_SRC_DREQ
							     | TI_WAIT_RESP
							     | TI_INTEN;
		m_pControlBlock[nID]->nSourceAddress = (m_nIOAddress & 0xFFFFFF) + GPU_IO_BASE;
		m_pControlBlock[nID]->nDestinationAddress = BUS_ADDRESS ((uintptr) m_pDMABuffer[nID]);
		m_pControlBlock[nID]->nTransferLength = m_nChunkSize * sizeof (u32);
	}

	m_pControlBlock[nID]->n2DModeStride = 0;
	m_pControlBlock[nID]->nReserved[0]  = 0;
	m_pControlBlock[nID]->nReserved[1]  = 0;

	return TRUE;
}
