//
// dmachannel.cpp
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
#include <circle/dmachannel.h>
#include <circle/bcm2835.h>
#include <circle/bcm2835int.h>
#include <circle/memio.h>
#include <circle/timer.h>
#include <circle/synchronize.h>
#include <assert.h>

#define DMA_CHANNELS			7	// only channels 0-6 are supported so far

#define ARM_DMACHAN_CS(chan)		(ARM_DMA_BASE + ((chan) * 0x100) + 0x00)
	#define CS_RESET			(1 << 31)
	#define CS_ABORT			(1 << 30)
	#define CS_WAIT_FOR_OUTSTANDING_WRITES	(1 << 28)
	#define CS_PANIC_PRIORITY_SHIFT		20
		#define DEFAULT_PANIC_PRIORITY		15
	#define CS_PRIORITY_SHIFT		16
		#define DEFAULT_PRIORITY		1
	#define CS_ERROR			(1 << 8)
	#define CS_INT				(1 << 2)
	#define CS_END				(1 << 1)
	#define CS_ACTIVE			(1 << 0)
#define ARM_DMACHAN_CONBLK_AD(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x04)
#define ARM_DMACHAN_TI(chan)		(ARM_DMA_BASE + ((chan) * 0x100) + 0x08)
	#define TI_PERMAP_SHIFT			16
	#define TI_BURST_LENGTH_SHIFT		12
		#define DEFAULT_BURST_LENGTH		0
	#define TI_SRC_IGNORE			(1 << 11)
	#define TI_SRC_DREQ			(1 << 10)
	#define TI_SRC_WIDTH			(1 << 9)
	#define TI_SRC_INC			(1 << 8)
	#define TI_DEST_DREQ			(1 << 6)
	#define TI_DEST_WIDTH			(1 << 5)
	#define TI_DEST_INC			(1 << 4)
	#define TI_WAIT_RESP			(1 << 3)
	#define TI_TDMODE			(1 << 1)
	#define TI_INTEN			(1 << 0)
#define ARM_DMACHAN_SOURCE_AD(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x0C)
#define ARM_DMACHAN_DEST_AD(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x10)
#define ARM_DMACHAN_TXFR_LEN(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x14)
	#define TXFR_LEN_XLENGTH_SHIFT		0
	#define TXFR_LEN_YLENGTH_SHIFT		16
	#define TXFR_LEN_MAX			0x3FFFFFFF
#define ARM_DMACHAN_STRIDE(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x18)
	#define STRIDE_SRC_SHIFT		0
	#define STRIDE_DEST_SHIFT		16
#define ARM_DMACHAN_NEXTCONBK(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x1C)
#define ARM_DMACHAN_DEBUG(chan)		(ARM_DMA_BASE + ((chan) * 0x100) + 0x20)
	#define DEBUG_LITE			(1 << 28)
#define ARM_DMA_INT_STATUS		(ARM_DMA_BASE + 0xFE0)
#define ARM_DMA_ENABLE			(ARM_DMA_BASE + 0xFF0)

CDMAChannel::CDMAChannel (unsigned nChannel, CInterruptSystem *pInterruptSystem)
:	m_nChannel (nChannel),
	m_pControlBlockBuffer (0),
	m_pControlBlock (0),
	m_pInterruptSystem (pInterruptSystem),
	m_bIRQConnected (FALSE),
	m_pCompletionRoutine (0),
	m_pCompletionParam (0)
{
	PeripheralEntry ();

	assert (m_nChannel < DMA_CHANNELS);
	assert (!(read32 (ARM_DMACHAN_DEBUG (m_nChannel)) & DEBUG_LITE));

	m_pControlBlockBuffer = new u8[sizeof (TDMAControlBlock) + 31];
	assert (m_pControlBlockBuffer != 0);

	m_pControlBlock = (TDMAControlBlock *) (((u32) m_pControlBlockBuffer + 31) & ~31);
	m_pControlBlock->nReserved[0] = 0;
	m_pControlBlock->nReserved[1] = 0;

	write32 (ARM_DMA_ENABLE, read32 (ARM_DMA_ENABLE) | (1 << m_nChannel));
	CTimer::Get ()->usDelay (1000);

	write32 (ARM_DMACHAN_CS (m_nChannel), CS_RESET);
	while (read32 (ARM_DMACHAN_CS (m_nChannel)) & CS_RESET)
	{
		// do nothing
	}

	PeripheralExit ();
}
	

CDMAChannel::~CDMAChannel (void)
{
	m_pCompletionRoutine = 0;

	if (m_pInterruptSystem != 0)
	{
		if (m_bIRQConnected)
		{
			assert (m_nChannel <= 12);
			m_pInterruptSystem->DisconnectIRQ (ARM_IRQ_DMA0+m_nChannel);
		}

		m_pInterruptSystem = 0;
	}

	m_pControlBlock = 0;

	delete [] m_pControlBlockBuffer;
	m_pControlBlockBuffer = 0;
}

void CDMAChannel::SetupMemCopy (void *pDestination, const void *pSource, size_t nLength)
{
	assert (pDestination != 0);
	assert (pSource != 0);
	assert (nLength > 0);

	assert (m_pControlBlock != 0);
	assert (nLength <= TXFR_LEN_MAX);

	m_pControlBlock->nTransferInformation     =   (DEFAULT_BURST_LENGTH << TI_BURST_LENGTH_SHIFT)
						    | TI_SRC_WIDTH
						    | TI_SRC_INC
						    | TI_DEST_WIDTH
						    | TI_DEST_INC;
	m_pControlBlock->nSourceAddress           = (u32) pSource + GPU_MEM_BASE;
	m_pControlBlock->nDestinationAddress      = (u32) pDestination + GPU_MEM_BASE;
	m_pControlBlock->nTransferLength          = nLength;
	m_pControlBlock->n2DModeStride            = 0;
	m_pControlBlock->nNextControlBlockAddress = 0;

	m_nDestinationAddress = (u32) pDestination;
	m_nBufferLength = nLength;

	CleanAndInvalidateDataCacheRange ((u32) pSource, nLength);
	CleanAndInvalidateDataCacheRange ((u32) pDestination, nLength);
}

void CDMAChannel::SetupIORead (void *pDestination, u32 nIOAddress, size_t nLength, TDREQ DREQ)
{
	assert (pDestination != 0);
	assert (nLength > 0);
	assert (nLength <= TXFR_LEN_MAX);

	nIOAddress &= 0xFFFFFF;
	assert (nIOAddress != 0);
	nIOAddress += GPU_IO_BASE;

	assert (m_pControlBlock != 0);
	m_pControlBlock->nTransferInformation     =   (DREQ << TI_PERMAP_SHIFT)
						    | (DEFAULT_BURST_LENGTH << TI_BURST_LENGTH_SHIFT)
						    | TI_SRC_DREQ
						    | TI_DEST_WIDTH
						    | TI_DEST_INC
						    | TI_WAIT_RESP;
	m_pControlBlock->nSourceAddress           = nIOAddress;
	m_pControlBlock->nDestinationAddress      = (u32) pDestination + GPU_MEM_BASE;
	m_pControlBlock->nTransferLength          = nLength;
	m_pControlBlock->n2DModeStride            = 0;
	m_pControlBlock->nNextControlBlockAddress = 0;

	m_nDestinationAddress = (u32) pDestination;
	m_nBufferLength = nLength;

	CleanAndInvalidateDataCacheRange ((u32) pDestination, nLength);
}

void CDMAChannel::SetupIOWrite (u32 nIOAddress, const void *pSource, size_t nLength, TDREQ DREQ)
{
	assert (pSource != 0);
	assert (nLength > 0);
	assert (nLength <= TXFR_LEN_MAX);

	nIOAddress &= 0xFFFFFF;
	assert (nIOAddress != 0);
	nIOAddress += GPU_IO_BASE;

	assert (m_pControlBlock != 0);
	m_pControlBlock->nTransferInformation     =   (DREQ << TI_PERMAP_SHIFT)
						    | (DEFAULT_BURST_LENGTH << TI_BURST_LENGTH_SHIFT)
						    | TI_SRC_WIDTH
						    | TI_SRC_INC
						    | TI_DEST_DREQ
						    | TI_WAIT_RESP;
	m_pControlBlock->nSourceAddress           = (u32) pSource + GPU_MEM_BASE;
	m_pControlBlock->nDestinationAddress      = nIOAddress;
	m_pControlBlock->nTransferLength          = nLength;
	m_pControlBlock->n2DModeStride            = 0;
	m_pControlBlock->nNextControlBlockAddress = 0;

	m_nDestinationAddress = 0;

	CleanAndInvalidateDataCacheRange ((u32) pSource, nLength);
}

void CDMAChannel::SetCompletionRoutine (TDMACompletionRoutine *pRoutine, void *pParam)
{
	assert (m_nChannel <= 12);
	assert (m_pInterruptSystem != 0);

	if (!m_bIRQConnected)
	{
		m_pInterruptSystem->ConnectIRQ (ARM_IRQ_DMA0+m_nChannel, InterruptStub, this);

		m_bIRQConnected = TRUE;
	}

	m_pCompletionRoutine = pRoutine;
	assert (m_pCompletionRoutine != 0);

	m_pCompletionParam = pParam;
}

void CDMAChannel::Start (void)
{
	assert (m_nChannel < DMA_CHANNELS);
	assert (m_pControlBlock != 0);

	if (m_pCompletionRoutine != 0)
	{
		assert (m_pInterruptSystem != 0);
		assert (m_bIRQConnected);
		m_pControlBlock->nTransferInformation |= TI_INTEN;
	}

	PeripheralEntry ();

	assert (!(read32 (ARM_DMACHAN_CS (m_nChannel)) & CS_INT));
	assert (!(read32 (ARM_DMA_INT_STATUS) & (1 << m_nChannel)));

	write32 (ARM_DMACHAN_CONBLK_AD (m_nChannel), (u32) m_pControlBlock + GPU_MEM_BASE);

	CleanAndInvalidateDataCacheRange ((u32) m_pControlBlock, sizeof *m_pControlBlock);

	write32 (ARM_DMACHAN_CS (m_nChannel),   CS_WAIT_FOR_OUTSTANDING_WRITES
					      | (DEFAULT_PANIC_PRIORITY << CS_PANIC_PRIORITY_SHIFT)
					      | (DEFAULT_PRIORITY << CS_PRIORITY_SHIFT)
					      | CS_ACTIVE);

	PeripheralExit ();
}

boolean CDMAChannel::Wait (void)
{
	assert (m_nChannel < DMA_CHANNELS);
	assert (m_pCompletionRoutine == 0);

	PeripheralEntry ();

	u32 nCS;
	while ((nCS = read32 (ARM_DMACHAN_CS (m_nChannel))) & CS_ACTIVE)
	{
		// do nothing
	}

	m_bStatus = nCS & CS_ERROR ? FALSE : TRUE;

	if (m_nDestinationAddress != 0)
	{
		CleanAndInvalidateDataCacheRange (m_nDestinationAddress, m_nBufferLength);
	}

	PeripheralExit ();

	return m_bStatus;
}

boolean CDMAChannel::GetStatus (void)
{
	assert (m_nChannel < DMA_CHANNELS);
	assert (!(read32 (ARM_DMACHAN_CS (m_nChannel)) & CS_ACTIVE));

	return m_bStatus;
}

void CDMAChannel::InterruptHandler (void)
{
	if (m_nDestinationAddress != 0)
	{
		CleanAndInvalidateDataCacheRange (m_nDestinationAddress, m_nBufferLength);
	}

	PeripheralEntry ();

	assert (m_nChannel < DMA_CHANNELS);

	u32 nIntStatus = read32 (ARM_DMA_INT_STATUS);
	u32 nIntMask = 1 << m_nChannel;
	assert (nIntStatus & nIntMask);
	write32 (ARM_DMA_INT_STATUS, nIntMask);

	u32 nCS = read32 (ARM_DMACHAN_CS (m_nChannel));
	assert (nCS & CS_INT);
	assert (!(nCS & CS_ACTIVE));
	write32 (ARM_DMACHAN_CS (m_nChannel), CS_INT); 

	PeripheralExit ();

	m_bStatus = nCS & CS_ERROR ? FALSE : TRUE;

	assert (m_pCompletionRoutine != 0);
	(*m_pCompletionRoutine) (m_nChannel, m_bStatus, m_pCompletionParam);
}

void CDMAChannel::InterruptStub (void *pParam)
{
	CDMAChannel *pThis = (CDMAChannel *) pParam;
	assert (pThis != 0);

	pThis->InterruptHandler ();
}
