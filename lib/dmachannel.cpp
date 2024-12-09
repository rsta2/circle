//
// dmachannel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2024  R. Stange <rsta2@o2online.de>
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
#include <circle/new.h>
#include <assert.h>

#define DMA_CHANNELS			(DMA_CHANNEL_MAX + 1)

CDMAChannel::CDMAChannel (unsigned nChannel, CInterruptSystem *pInterruptSystem)
:	m_nChannel (CMachineInfo::Get ()->AllocateDMAChannel (nChannel)),
	m_nBuffers (0),
	m_pInterruptSystem (pInterruptSystem),
	m_bIRQConnected (FALSE),
	m_pCompletionRoutine (0),
	m_pCompletionParam (0),
	m_bStatus (FALSE)
{
#if RASPPI >= 4
	m_pDMA4Channel = 0;
	if (   nChannel == DMA_CHANNEL_EXTENDED
	    || (   nChannel >= DMA_CHANNEL_EXT_MIN
		&& nChannel <= DMA_CHANNEL_EXT_MAX))
	{
		assert (m_nChannel != DMA_CHANNEL_NONE);
		m_pDMA4Channel = new CDMA4Channel (m_nChannel, m_pInterruptSystem);
		assert (m_pDMA4Channel != 0);

		return;
	}
#endif

	PeripheralEntry ();

	assert (m_nChannel != DMA_CHANNEL_NONE);
	assert (m_nChannel < DMA_CHANNELS);

	for (unsigned i = 0; i < MaxCyclicBuffers; i++)
	{
		m_pControlBlock[i] = new (HEAP_DMA30) TDMAControlBlock;
		assert (m_pControlBlock[i]);

		m_pControlBlock[i]->nReserved[0] = 0;
		m_pControlBlock[i]->nReserved[1] = 0;
	}

	write32 (ARM_DMA_ENABLE, read32 (ARM_DMA_ENABLE) | (1 << m_nChannel));
	CTimer::SimpleusDelay (1000);

	write32 (ARM_DMACHAN_CS (m_nChannel), CS_RESET);
	while (read32 (ARM_DMACHAN_CS (m_nChannel)) & CS_RESET)
	{
		// do nothing
	}

	PeripheralExit ();
}
	

CDMAChannel::~CDMAChannel (void)
{
#if RASPPI >= 4
	if (m_pDMA4Channel != 0)
	{
		assert (DMA_CHANNEL_EXT_MIN <= m_nChannel && m_nChannel <= DMA_CHANNEL_EXT_MAX);

		delete m_pDMA4Channel;
		m_pDMA4Channel = 0;

		CMachineInfo::Get ()->FreeDMAChannel (m_nChannel);

		return;
	}
#endif

	PeripheralEntry ();

	assert (m_nChannel < DMA_CHANNELS);

	write32 (ARM_DMACHAN_CS (m_nChannel), CS_RESET);
	while (read32 (ARM_DMACHAN_CS (m_nChannel)) & CS_RESET)
	{
		// do nothing
	}

	write32 (ARM_DMA_ENABLE, read32 (ARM_DMA_ENABLE) & ~(1 << m_nChannel));

	PeripheralExit ();

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

	CMachineInfo::Get ()->FreeDMAChannel (m_nChannel);

	for (int i = 0; i < MaxCyclicBuffers; i++)
	{
		delete m_pControlBlock[i];
		m_pControlBlock[i] = 0;
	}
}

void CDMAChannel::SetupMemCopy (void *pDestination, const void *pSource, size_t nLength,
				unsigned nBurstLength, boolean bCached)
{
#if RASPPI >= 4
	if (m_pDMA4Channel != 0)
	{
		m_pDMA4Channel->SetupMemCopy (pDestination, pSource, nLength, nBurstLength, bCached);

		return;
	}
#endif

	assert (pDestination != 0);
	assert (pSource != 0);
	assert (nLength > 0);
	assert (nBurstLength <= 15);

	assert (m_pControlBlock[0] != 0);
	assert (nLength <= TXFR_LEN_MAX);
	assert (   !(read32 (ARM_DMACHAN_DEBUG (m_nChannel)) & DEBUG_LITE)
		|| nLength <= TXFR_LEN_MAX_LITE);

	m_pControlBlock[0]->nTransferInformation     =   (nBurstLength << TI_BURST_LENGTH_SHIFT)
						       | TI_SRC_WIDTH
						       | TI_SRC_INC
						       | TI_DEST_WIDTH
						       | TI_DEST_INC;
	m_pControlBlock[0]->nSourceAddress           = BUS_ADDRESS ((uintptr) pSource);
	m_pControlBlock[0]->nDestinationAddress      = BUS_ADDRESS ((uintptr) pDestination);
	m_pControlBlock[0]->nTransferLength          = nLength;
	m_pControlBlock[0]->n2DModeStride            = 0;
	m_pControlBlock[0]->nNextControlBlockAddress = 0;

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

	m_nBuffers = 1;
}

void CDMAChannel::SetupIORead (void *pDestination, uintptr ulIOAddress, size_t nLength, TDREQ DREQ)
{
#if RASPPI >= 4
	if (m_pDMA4Channel != 0)
	{
		m_pDMA4Channel->SetupIORead (pDestination, ulIOAddress, nLength, DREQ);

		return;
	}
#endif

	assert (pDestination != 0);
	assert (nLength > 0);
	assert (nLength <= TXFR_LEN_MAX);
	assert (   !(read32 (ARM_DMACHAN_DEBUG (m_nChannel)) & DEBUG_LITE)
		|| nLength <= TXFR_LEN_MAX_LITE);

	ulIOAddress &= 0xFFFFFF;
	assert (ulIOAddress != 0);
	ulIOAddress += GPU_IO_BASE;

	assert (m_pControlBlock[0] != 0);
	m_pControlBlock[0]->nTransferInformation     =   (DREQ << TI_PERMAP_SHIFT)
						       | (DEFAULT_BURST_LENGTH << TI_BURST_LENGTH_SHIFT)
						       | TI_SRC_DREQ
						       | TI_DEST_WIDTH
						       | TI_DEST_INC
						       | TI_WAIT_RESP;
	m_pControlBlock[0]->nSourceAddress           = ulIOAddress;
	m_pControlBlock[0]->nDestinationAddress      = BUS_ADDRESS ((uintptr) pDestination);
	m_pControlBlock[0]->nTransferLength          = nLength;
	m_pControlBlock[0]->n2DModeStride            = 0;
	m_pControlBlock[0]->nNextControlBlockAddress = 0;

	m_nDestinationAddress = (uintptr) pDestination;
	m_nBufferLength = nLength;

	CleanAndInvalidateDataCacheRange ((uintptr) pDestination, nLength);

	m_nBuffers = 1;
}

void CDMAChannel::SetupIOWrite (uintptr ulIOAddress, const void *pSource, size_t nLength, TDREQ DREQ)
{
#if RASPPI >= 4
	if (m_pDMA4Channel != 0)
	{
		m_pDMA4Channel->SetupIOWrite (ulIOAddress, pSource, nLength, DREQ);

		return;
	}
#endif

	assert (pSource != 0);
	assert (nLength > 0);
	assert (nLength <= TXFR_LEN_MAX);
	assert (   !(read32 (ARM_DMACHAN_DEBUG (m_nChannel)) & DEBUG_LITE)
		|| nLength <= TXFR_LEN_MAX_LITE);

	ulIOAddress &= 0xFFFFFF;
	assert (ulIOAddress != 0);
	ulIOAddress += GPU_IO_BASE;

	assert (m_pControlBlock[0] != 0);
	m_pControlBlock[0]->nTransferInformation     =   (DREQ << TI_PERMAP_SHIFT)
						       | (DEFAULT_BURST_LENGTH << TI_BURST_LENGTH_SHIFT)
						       | TI_SRC_WIDTH
						       | TI_SRC_INC
						       | TI_DEST_DREQ
						       | TI_WAIT_RESP;
	m_pControlBlock[0]->nSourceAddress           = BUS_ADDRESS ((uintptr) pSource);
	m_pControlBlock[0]->nDestinationAddress      = ulIOAddress;
	m_pControlBlock[0]->nTransferLength          = nLength;
	m_pControlBlock[0]->n2DModeStride            = 0;
	m_pControlBlock[0]->nNextControlBlockAddress = 0;

	m_nDestinationAddress = 0;

	CleanAndInvalidateDataCacheRange ((uintptr) pSource, nLength);

	m_nBuffers = 1;
}

void CDMAChannel::SetupCyclicIOWrite (uintptr ulIOAddress, const void *ppSources[],
				      unsigned nBuffers, size_t ulLength, TDREQ DREQ)
{
#if RASPPI >= 4
	if (m_pDMA4Channel != 0)
	{
		m_pDMA4Channel->SetupCyclicIOWrite (ulIOAddress, ppSources, nBuffers,
						    ulLength, DREQ);

		return;
	}
#endif

	assert (ppSources != 0);
	assert (ulLength > 0);
	assert (ulLength <= TXFR_LEN_MAX);
	assert (   !(read32 (ARM_DMACHAN_DEBUG (m_nChannel)) & DEBUG_LITE)
		|| ulLength <= TXFR_LEN_MAX_LITE);

	ulIOAddress &= 0xFFFFFF;
	assert (ulIOAddress != 0);
	ulIOAddress += GPU_IO_BASE;

	for (unsigned i = 0; i < nBuffers; i++)
	{
		assert (ppSources[i]);
		assert (m_pControlBlock[i] != 0);
		m_pControlBlock[i]->nTransferInformation     =   (DREQ << TI_PERMAP_SHIFT)
							       | (DEFAULT_BURST_LENGTH << TI_BURST_LENGTH_SHIFT)
							       | TI_SRC_WIDTH
							       | TI_SRC_INC
							       | TI_DEST_DREQ
							       | TI_WAIT_RESP;
		m_pControlBlock[i]->nSourceAddress           = BUS_ADDRESS ((uintptr) ppSources[i]);
		m_pControlBlock[i]->nDestinationAddress      = ulIOAddress;
		m_pControlBlock[i]->nTransferLength          = ulLength;
		m_pControlBlock[i]->n2DModeStride            = 0;

		if (nBuffers > 1)
		{
			m_pControlBlock[i]->nNextControlBlockAddress =
				BUS_ADDRESS ((uintptr) m_pControlBlock[i == nBuffers-1 ? 0 : i+1]);
		}
		else
		{
			m_pControlBlock[i]->nNextControlBlockAddress = 0;
		}

		m_nDestinationAddress = 0;

		CleanAndInvalidateDataCacheRange ((uintptr) ppSources[i], ulLength);

		m_pBuffer[i] = ppSources[i];
	}

	m_nBuffers = nBuffers;
	m_nBufferLength = ulLength;
}

void CDMAChannel::SetupMemCopy2D (void *pDestination, const void *pSource,
				  size_t nBlockLength, unsigned nBlockCount, size_t nBlockStride,
				  unsigned nBurstLength)
{
#if RASPPI >= 4
	if (m_pDMA4Channel != 0)
	{
		m_pDMA4Channel->SetupMemCopy2D (pDestination, pSource, nBlockLength,
						nBlockCount, nBlockStride, nBurstLength);

		return;
	}
#endif

	assert (pDestination != 0);
	assert (pSource != 0);
	assert (nBlockLength > 0);
	assert (nBlockLength <= 0xFFFF);
	assert (nBlockCount > 0);
	assert (nBlockCount <= 0x3FFF);
	assert (nBlockStride <= 0xFFFF);
	assert (nBurstLength <= 15);

	assert (!(read32 (ARM_DMACHAN_DEBUG (m_nChannel)) & DEBUG_LITE));

	assert (m_pControlBlock[0] != 0);

	m_pControlBlock[0]->nTransferInformation     =   (nBurstLength << TI_BURST_LENGTH_SHIFT)
						       | TI_SRC_WIDTH
						       | TI_SRC_INC
						       | TI_DEST_WIDTH
						       | TI_DEST_INC
						       | TI_TDMODE;
	m_pControlBlock[0]->nSourceAddress           = BUS_ADDRESS ((uintptr) pSource);
	m_pControlBlock[0]->nDestinationAddress      = BUS_ADDRESS ((uintptr) pDestination);
	m_pControlBlock[0]->nTransferLength          =   ((nBlockCount-1) << TXFR_LEN_YLENGTH_SHIFT)
						       | (nBlockLength << TXFR_LEN_XLENGTH_SHIFT);
	m_pControlBlock[0]->n2DModeStride            = nBlockStride << STRIDE_DEST_SHIFT;
	m_pControlBlock[0]->nNextControlBlockAddress = 0;

	m_nDestinationAddress = 0;

	CleanAndInvalidateDataCacheRange ((uintptr) pSource, nBlockLength*nBlockCount);

	m_nBuffers = 1;
}

void CDMAChannel::SetCompletionRoutine (TDMACompletionRoutine *pRoutine, void *pParam)
{
#if RASPPI >= 4
	if (m_pDMA4Channel != 0)
	{
		m_pDMA4Channel->SetCompletionRoutine (pRoutine, pParam);

		return;
	}
#endif

	assert (m_nChannel <= DMA_CHANNELS);
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
#if RASPPI >= 4
	if (m_pDMA4Channel != 0)
	{
		m_pDMA4Channel->Start ();

		return;
	}
#endif

	for (unsigned i = 0; i < m_nBuffers; i++)
	{
		assert (m_pControlBlock[i] != 0);

		if (m_pCompletionRoutine != 0)
		{
			assert (m_pInterruptSystem != 0);
			assert (m_bIRQConnected);
			m_pControlBlock[i]->nTransferInformation |= TI_INTEN;
		}

		CleanAndInvalidateDataCacheRange ((uintptr) m_pControlBlock[i],
						  sizeof *m_pControlBlock[i]);
	}

	PeripheralEntry ();

	assert (m_nChannel < DMA_CHANNELS);
	assert (!(read32 (ARM_DMACHAN_CS (m_nChannel)) & CS_INT));
	assert (!(read32 (ARM_DMA_INT_STATUS) & (1 << m_nChannel)));

	m_nCurrentBuffer = 0;
	write32 (ARM_DMACHAN_CONBLK_AD (m_nChannel), BUS_ADDRESS ((uintptr) m_pControlBlock[0]));

	write32 (ARM_DMACHAN_CS (m_nChannel),   CS_WAIT_FOR_OUTSTANDING_WRITES
					      | (DEFAULT_PANIC_PRIORITY << CS_PANIC_PRIORITY_SHIFT)
					      | (DEFAULT_PRIORITY << CS_PRIORITY_SHIFT)
					      | CS_ACTIVE);

	PeripheralExit ();
}

boolean CDMAChannel::Wait (void)
{
#if RASPPI >= 4
	if (m_pDMA4Channel != 0)
	{
		return m_pDMA4Channel->Wait ();
	}
#endif

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
#if RASPPI >= 4
	if (m_pDMA4Channel != 0)
	{
		return m_pDMA4Channel->GetStatus ();
	}
#endif

	assert (m_nChannel < DMA_CHANNELS);
	assert (!(read32 (ARM_DMACHAN_CS (m_nChannel)) & CS_ACTIVE));

	return m_bStatus;
}

void CDMAChannel::Cancel (void)
{
#if RASPPI >= 4
	if (m_pDMA4Channel != 0)
	{
		m_pDMA4Channel->Cancel ();

		return;
	}
#endif

	PeripheralEntry ();

	assert (m_nChannel < DMA_CHANNELS);
	write32 (ARM_DMACHAN_CS (m_nChannel), 0);

	PeripheralExit ();
}

void CDMAChannel::InterruptHandler (void)
{
	if (m_nDestinationAddress != 0)
	{
		CleanAndInvalidateDataCacheRange (m_nDestinationAddress, m_nBufferLength);
	}

	PeripheralEntry ();

	assert (m_nChannel < DMA_CHANNELS);

#ifndef NDEBUG
	u32 nIntStatus = read32 (ARM_DMA_INT_STATUS);
#endif
	u32 nIntMask = 1 << m_nChannel;
	assert (nIntStatus & nIntMask);
	write32 (ARM_DMA_INT_STATUS, nIntMask);

	u32 nCS = read32 (ARM_DMACHAN_CS (m_nChannel));
	assert (nCS & CS_INT);
	write32 (ARM_DMACHAN_CS (m_nChannel), nCS);

	PeripheralExit ();

	m_bStatus = nCS & CS_ERROR ? FALSE : TRUE;

	assert (m_pCompletionRoutine != 0);
	TDMACompletionRoutine *pCompletionRoutine = m_pCompletionRoutine;
	m_pCompletionRoutine = 0;

	assert (m_nCurrentBuffer < MaxCyclicBuffers);
	(*pCompletionRoutine) (m_nChannel, m_nCurrentBuffer, m_bStatus, m_pCompletionParam);

	if (   m_bStatus
	    && m_nBuffers > 1)
	{
		assert (m_nCurrentBuffer < m_nBuffers);

		CleanAndInvalidateDataCacheRange ((uintptr) m_pBuffer[m_nCurrentBuffer],
						  m_nBufferLength);

		if (++m_nCurrentBuffer == m_nBuffers)
		{
			m_nCurrentBuffer = 0;
		}
	}
}

void CDMAChannel::InterruptStub (void *pParam)
{
	CDMAChannel *pThis = (CDMAChannel *) pParam;
	assert (pThis != 0);

	pThis->InterruptHandler ();
}
