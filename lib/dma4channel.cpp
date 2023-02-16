//
// dmachannel.cpp
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
#include <circle/dma4channel.h>
#include <circle/bcm2711.h>
#include <circle/bcm2711int.h>
#include <circle/memio.h>
#include <circle/timer.h>
#include <circle/synchronize.h>
#include <circle/new.h>
#include <assert.h>

#define DMA4_CHANNEL_MIN		11
#define DMA4_CHANNEL_MAX		14

#define ARM_DMA4CHAN_CS(chan)		(ARM_DMA_BASE + ((chan) * 0x100) + 0x00)
	#define CS4_HALT			(1 << 31)
	#define CS4_ABORT			(1 << 30)
	#define CS4_WAIT_FOR_OUTSTANDING_WRITES	(1 << 28)
	#define CS4_PANIC_QOS_SHIFT		20
		#define DEFAULT_PANIC_QOS4		15
	#define CS4_QOS_SHIFT			16
		#define DEFAULT_QOS4			1
	#define CS4_ERROR			(1 << 10)
	#define CS4_INT				(1 << 2)
	#define CS4_END				(1 << 1)
	#define CS4_ACTIVE			(1 << 0)
#define ARM_DMA4CHAN_CONBLK_AD(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x04)
	#define CONBLK_AD4_ADDR_SHIFT		5
#define ARM_DMA4CHAN_DEBUG(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x0C)
	#define DEBUG4_VERSION_SHIFT		28
	#define DEBUG4_VERSION_MASK		(0xF << 28)
		#define DMA4_VERSION			1
	#define DEBUG4_RESET			(1 << 23)
#define ARM_DMA4CHAN_TI(chan)		(ARM_DMA_BASE + ((chan) * 0x100) + 0x10)
	#define TI4_DEST_DREQ			(1 << 15)
	#define TI4_SRC_DREQ			(1 << 14)
	#define TI4_PERMAP_SHIFT		9
	#define TI4_WAIT_RD_RESP		(1 << 3)
	#define TI4_WAIT_RESP			(1 << 2)
	#define TI4_TDMODE			(1 << 1)
	#define TI4_INTEN			(1 << 0)
#define ARM_DMA4CHAN_SOURCE_AD(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x14)	// [31:0]
#define ARM_DMA4CHAN_SOURCE_INFO(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x18)
	#define SOURCE4_STRIDE_SHIFT		16
		#define SOURCE4_STRIDE_MAX		0xFFFF
	#define SOURCE4_IGNORE			(1 << 15)
	#define SOURCE4_SIZE_SHIFT		13
		#define SIZE4_128			2
		#define SIZE4_64			1
		#define SIZE4_32			0
	#define SOURCE4_INC			(1 << 12)
	#define SOURCE4_BURST_LEN_SHIFT		8
		#define BURST4_DEFAULT			0
		#define BURST4_MAX			15
	#define SOURCE4_ADDR_SHIFT		0					// [39:32]
		#define FULL35_ADDR_OFFSET		4	// for "large address" masters
#define ARM_DMA4CHAN_DEST_AD(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x1C)	// [31:0]
#define ARM_DMA4CHAN_DEST_INFO(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x20)
	#define DEST4_STRIDE_SHIFT		16
		#define DEST4_STRIDE_MAX		0xFFFF
	#define DEST4_IGNORE			(1 << 15)
	#define DEST4_SIZE_SHIFT		13
	#define DEST4_INC			(1 << 12)
	#define DEST4_BURST_LEN_SHIFT		8
	#define DEST4_ADDR_SHIFT		0					// [39:32]
#define ARM_DMA4CHAN_LEN(chan)		(ARM_DMA_BASE + ((chan) * 0x100) + 0x24)
	#define LEN4_YLENGTH_SHIFT		16
		#define LEN4_YLENGTH_MAX		0x3FFF
	#define LEN4_XLENGTH_SHIFT		0
		#define LEN4_XLENGTH_MAX		0x3FFFFFFF
		#define LEN4_XLENGTH_2D_MAX		0xFFFF
#define ARM_DMA4CHAN_NEXTCONBK(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x28)
#define ARM_DMA_INT_STATUS		(ARM_DMA_BASE + 0xFE0)
#define ARM_DMA_ENABLE			(ARM_DMA_BASE + 0xFF0)

#if AARCH == 32
	#define ADDRESS4_LOW(ptr)	((uintptr) (ptr))
	#define ADDRESS4_HIGH(ptr)	(0)
#else
	#define ADDRESS4_LOW(ptr)	((uintptr) (ptr) & 0xFFFFFFFFUL)
	#define ADDRESS4_HIGH(ptr)	((((uintptr) (ptr) >> 32) & 0xFF))
#endif

CDMA4Channel::CDMA4Channel (unsigned nChannel, CInterruptSystem *pInterruptSystem)
:	m_nChannel (nChannel),
	m_pControlBlockBuffer (0),
	m_pControlBlock (0),
	m_pInterruptSystem (pInterruptSystem),
	m_bIRQConnected (FALSE),
	m_pCompletionRoutine (0),
	m_pCompletionParam (0),
	m_bStatus (FALSE)
{
	PeripheralEntry();

	assert (m_nChannel >= DMA4_CHANNEL_MIN);
	assert (m_nChannel <= DMA4_CHANNEL_MAX);
	assert (   (read32 (ARM_DMA4CHAN_DEBUG (m_nChannel)) & DEBUG4_VERSION_MASK)
		>> DEBUG4_VERSION_SHIFT == DMA4_VERSION);

	m_pControlBlockBuffer = new (HEAP_ANY) u8[sizeof (TDMA4ControlBlock) + 31];
	assert (m_pControlBlockBuffer != 0);

	m_pControlBlock = (TDMA4ControlBlock *) (((uintptr) m_pControlBlockBuffer + 31) & ~31);
	m_pControlBlock->nReserved = 0;

	write32 (ARM_DMA4CHAN_CONBLK_AD (m_nChannel), 0);
	write32 (ARM_DMA_ENABLE, read32 (ARM_DMA_ENABLE) | (1 << m_nChannel));
	CTimer::SimpleusDelay (1000);

	write32 (ARM_DMA4CHAN_DEBUG (m_nChannel), DEBUG4_RESET);
	CTimer::SimpleusDelay (1000);

	PeripheralExit();
}

CDMA4Channel::~CDMA4Channel (void)
{
	assert (m_nChannel >= DMA4_CHANNEL_MIN);
	assert (m_nChannel <= DMA4_CHANNEL_MAX);

	write32 (ARM_DMA4CHAN_DEBUG (m_nChannel), DEBUG4_RESET);
	CTimer::SimpleusDelay (1000);

	write32 (ARM_DMA_ENABLE, read32 (ARM_DMA_ENABLE) & ~(1 << m_nChannel));

	m_pCompletionRoutine = 0;

	if (m_pInterruptSystem != 0)
	{
		if (m_bIRQConnected)
		{
			assert (m_nChannel >= 11);
			m_pInterruptSystem->DisconnectIRQ (ARM_IRQ_DMA11+m_nChannel-11);
		}

		m_pInterruptSystem = 0;
	}

	m_pControlBlock = 0;

	delete [] m_pControlBlockBuffer;
	m_pControlBlockBuffer = 0;
}

void CDMA4Channel::SetupMemCopy (void *pDestination, const void *pSource, size_t nLength,
				unsigned nBurstLength, boolean bCached)
{
	assert (pDestination != 0);
	assert (pSource != 0);
	assert (nLength > 0);
	assert (nBurstLength <= BURST4_MAX);

	assert (m_pControlBlock != 0);
	assert (nLength <= LEN4_XLENGTH_MAX);

	m_pControlBlock->nTransferInformation     =   0;
	m_pControlBlock->nSourceAddress           = ADDRESS4_LOW (pSource);
	m_pControlBlock->nSourceInformation	  =   (SIZE4_128 << SOURCE4_SIZE_SHIFT)
						    | SOURCE4_INC
						    | (nBurstLength << SOURCE4_BURST_LEN_SHIFT)
						    |    (ADDRESS4_HIGH (pSource)
						      << SOURCE4_ADDR_SHIFT);
	m_pControlBlock->nDestinationAddress      = ADDRESS4_LOW (pDestination);
	m_pControlBlock->nDestinationInformation  =   (SIZE4_128 << DEST4_SIZE_SHIFT)
						    | DEST4_INC
						    | (nBurstLength << DEST4_BURST_LEN_SHIFT)
						    |    (ADDRESS4_HIGH (pDestination)
						      << DEST4_ADDR_SHIFT);
	m_pControlBlock->nTransferLength          = nLength << LEN4_XLENGTH_SHIFT;
	m_pControlBlock->nNextControlBlockAddress = 0;

	if (bCached)
	{
		m_nDestinationAddress = (uintptr) pDestination;
		m_nBufferLength = nLength;

		CleanAndInvalidateDataCacheRange ((uintptr) pSource, nLength);
		CleanAndInvalidateDataCacheRange ((uintptr) pDestination, nLength);
	}
	else
	{
		m_nDestinationAddress = 0;
	}
}

void CDMA4Channel::SetupIORead (void *pDestination, u32 nIOAddress, size_t nLength, TDREQ DREQ)
{
	assert (pDestination != 0);
	assert (nLength > 0);
	assert (nLength <= LEN4_XLENGTH_MAX);

	nIOAddress &= 0xFFFFFF;
	assert (nIOAddress != 0);
	nIOAddress += GPU_IO_BASE;

	assert (m_pControlBlock != 0);
	m_pControlBlock->nTransferInformation     =   TI4_SRC_DREQ
						    | (DREQ << TI4_PERMAP_SHIFT)
						    | TI4_WAIT_RD_RESP
						    | TI4_WAIT_RESP;
	m_pControlBlock->nSourceAddress           = nIOAddress;
	m_pControlBlock->nSourceInformation	  =   (SIZE4_32 << SOURCE4_SIZE_SHIFT)
						    | (BURST4_DEFAULT << SOURCE4_BURST_LEN_SHIFT)
						    | (FULL35_ADDR_OFFSET << SOURCE4_ADDR_SHIFT);
	m_pControlBlock->nDestinationAddress      = ADDRESS4_LOW (pDestination);
	m_pControlBlock->nDestinationInformation  =   (SIZE4_128 << DEST4_SIZE_SHIFT)
						    | DEST4_INC
						    | (BURST4_DEFAULT << DEST4_BURST_LEN_SHIFT)
						    |    (ADDRESS4_HIGH (pDestination)
						      << DEST4_ADDR_SHIFT);
	m_pControlBlock->nTransferLength          = nLength << LEN4_XLENGTH_SHIFT;
	m_pControlBlock->nNextControlBlockAddress = 0;

	m_nDestinationAddress = (uintptr) pDestination;
	m_nBufferLength = nLength;

	CleanAndInvalidateDataCacheRange ((uintptr) pDestination, nLength);
}

void CDMA4Channel::SetupIOWrite (u32 nIOAddress, const void *pSource, size_t nLength, TDREQ DREQ)
{
	assert (pSource != 0);
	assert (nLength > 0);
	assert (nLength <= LEN4_XLENGTH_MAX);

	nIOAddress &= 0xFFFFFF;
	assert (nIOAddress != 0);
	nIOAddress += GPU_IO_BASE;

	assert (m_pControlBlock != 0);
	m_pControlBlock->nTransferInformation     =   TI4_DEST_DREQ
						    | (DREQ << TI4_PERMAP_SHIFT)
						    | TI4_WAIT_RD_RESP
						    | TI4_WAIT_RESP;
	m_pControlBlock->nSourceAddress           = ADDRESS4_LOW (pSource);
	m_pControlBlock->nSourceInformation	  =   (SIZE4_128 << SOURCE4_SIZE_SHIFT)
						    | SOURCE4_INC
						    | (BURST4_DEFAULT << SOURCE4_BURST_LEN_SHIFT)
						    |    (ADDRESS4_HIGH (pSource)
						      << SOURCE4_ADDR_SHIFT);
	m_pControlBlock->nDestinationAddress      = nIOAddress;
	m_pControlBlock->nDestinationInformation  =   (SIZE4_32 << DEST4_SIZE_SHIFT)
						    | (BURST4_DEFAULT << DEST4_BURST_LEN_SHIFT)
						    | (FULL35_ADDR_OFFSET << DEST4_ADDR_SHIFT);
	m_pControlBlock->nTransferLength          = nLength << LEN4_XLENGTH_SHIFT;
	m_pControlBlock->nNextControlBlockAddress = 0;

	m_nDestinationAddress = 0;

	CleanAndInvalidateDataCacheRange ((uintptr) pSource, nLength);
}

void CDMA4Channel::SetupMemCopy2D (void *pDestination, const void *pSource,
				  size_t nBlockLength, unsigned nBlockCount, size_t nBlockStride,
				  unsigned nBurstLength)
{
	assert (pDestination != 0);
	assert (pSource != 0);
	assert (nBlockLength > 0);
	assert (nBlockLength <= LEN4_XLENGTH_2D_MAX);
	assert (nBlockCount > 0);
	assert (nBlockCount <= LEN4_YLENGTH_MAX);
	assert (nBlockStride <= DEST4_STRIDE_MAX);
	assert (nBurstLength <= BURST4_MAX);

	assert (m_pControlBlock != 0);

	m_pControlBlock->nTransferInformation     =   TI4_WAIT_RD_RESP
						    | TI4_WAIT_RESP
						    | TI4_TDMODE;
	m_pControlBlock->nSourceAddress           = ADDRESS4_LOW (pSource);
	m_pControlBlock->nSourceInformation	  =   (SIZE4_128 << SOURCE4_SIZE_SHIFT)
						    | SOURCE4_INC
						    | (nBurstLength << SOURCE4_BURST_LEN_SHIFT)
						    |    (ADDRESS4_HIGH (pSource)
						      << SOURCE4_ADDR_SHIFT);
	m_pControlBlock->nDestinationAddress      = ADDRESS4_LOW (pDestination);
	m_pControlBlock->nDestinationInformation  =   (nBlockStride << DEST4_STRIDE_SHIFT)
						    | (SIZE4_128 << DEST4_SIZE_SHIFT)
						    | DEST4_INC
						    | (nBurstLength << DEST4_BURST_LEN_SHIFT)
						    |    (ADDRESS4_HIGH (pDestination)
						      << DEST4_ADDR_SHIFT);
	m_pControlBlock->nTransferLength          =   ((nBlockCount-1) << LEN4_YLENGTH_SHIFT)
						    | (nBlockLength << LEN4_XLENGTH_SHIFT);
	m_pControlBlock->nNextControlBlockAddress = 0;

	m_nDestinationAddress = 0;

	CleanAndInvalidateDataCacheRange ((uintptr) pSource, nBlockLength*nBlockCount);
}

void CDMA4Channel::SetCompletionRoutine (TDMACompletionRoutine *pRoutine, void *pParam)
{
	assert (m_nChannel >= DMA4_CHANNEL_MIN);
	assert (m_nChannel <= DMA4_CHANNEL_MAX);
	assert (m_pInterruptSystem != 0);

	if (!m_bIRQConnected)
	{
		assert (m_nChannel >= 11);
		m_pInterruptSystem->ConnectIRQ (ARM_IRQ_DMA11+m_nChannel-11, InterruptStub, this);

		m_bIRQConnected = TRUE;
	}

	m_pCompletionRoutine = pRoutine;
	assert (m_pCompletionRoutine != 0);

	m_pCompletionParam = pParam;
}

void CDMA4Channel::Start (void)
{
	assert (m_nChannel >= DMA4_CHANNEL_MIN);
	assert (m_nChannel <= DMA4_CHANNEL_MAX);
	assert (m_pControlBlock != 0);

	if (m_pCompletionRoutine != 0)
	{
		assert (m_pInterruptSystem != 0);
		assert (m_bIRQConnected);
		m_pControlBlock->nTransferInformation |= TI4_INTEN;
	}

	PeripheralEntry ();

	assert (!(read32 (ARM_DMA4CHAN_CS (m_nChannel)) & CS4_INT));
	assert (!(read32 (ARM_DMA_INT_STATUS) & (1 << m_nChannel)));

	write32 (ARM_DMA4CHAN_CONBLK_AD (m_nChannel),
		 (uintptr) m_pControlBlock >> CONBLK_AD4_ADDR_SHIFT);

	CleanAndInvalidateDataCacheRange ((uintptr) m_pControlBlock, sizeof *m_pControlBlock);

	write32 (ARM_DMA4CHAN_CS (m_nChannel),   CS4_WAIT_FOR_OUTSTANDING_WRITES
					      | (DEFAULT_PANIC_QOS4 << CS4_PANIC_QOS_SHIFT)
					      | (DEFAULT_QOS4 << CS4_QOS_SHIFT)
					      | CS4_ACTIVE);

	PeripheralExit ();
}

boolean CDMA4Channel::Wait (void)
{
	assert (m_nChannel >= DMA4_CHANNEL_MIN);
	assert (m_nChannel <= DMA4_CHANNEL_MAX);
	assert (m_pCompletionRoutine == 0);

	PeripheralEntry ();

	u32 nCS;
	while ((nCS = read32 (ARM_DMA4CHAN_CS (m_nChannel))) & CS4_ACTIVE)
	{
		// do nothing
	}

	m_bStatus = nCS & CS4_ERROR ? FALSE : TRUE;

	if (m_nDestinationAddress != 0)
	{
		CleanAndInvalidateDataCacheRange (m_nDestinationAddress, m_nBufferLength);
	}

	PeripheralExit();

	return m_bStatus;
}

boolean CDMA4Channel::GetStatus (void)
{
	assert (m_nChannel >= DMA4_CHANNEL_MIN);
	assert (m_nChannel <= DMA4_CHANNEL_MAX);
	assert (!(read32 (ARM_DMA4CHAN_CS (m_nChannel)) & CS4_ACTIVE));

	return m_bStatus;
}

void CDMA4Channel::InterruptHandler (void)
{
	if (m_nDestinationAddress != 0)
	{
		CleanAndInvalidateDataCacheRange (m_nDestinationAddress, m_nBufferLength);
	}

	assert (m_nChannel >= DMA4_CHANNEL_MIN);
	assert (m_nChannel <= DMA4_CHANNEL_MAX);

#ifndef NDEBUG
	u32 nIntStatus = read32 (ARM_DMA_INT_STATUS);
#endif
	u32 nIntMask = 1 << m_nChannel;
	assert (nIntStatus & nIntMask);
	write32 (ARM_DMA_INT_STATUS, nIntMask);

	u32 nCS = read32 (ARM_DMA4CHAN_CS (m_nChannel));
	assert (nCS & CS4_INT);
	assert (!(nCS & CS4_ACTIVE));
	write32 (ARM_DMA4CHAN_CS (m_nChannel), CS4_INT);

	m_bStatus = nCS & CS4_ERROR ? FALSE : TRUE;

	assert (m_pCompletionRoutine != 0);
	(*m_pCompletionRoutine) (m_nChannel, m_bStatus, m_pCompletionParam);
}

void CDMA4Channel::InterruptStub (void *pParam)
{
	CDMA4Channel *pThis = (CDMA4Channel *) pParam;
	assert (pThis != 0);

	pThis->InterruptHandler ();
}
