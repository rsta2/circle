//
// dmacontrolblock.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/dmacontrolblock.h>
#include <circle/bcm2835.h>
#include <circle/bcm2835int.h>
#include <circle/memio.h>
#include <circle/timer.h>
#include <circle/synchronize.h>
#include <circle/new.h>
#include <assert.h>
#include <circle/logger.h>

// HACK for strcpy
// #include <circle/util.h>


LOGMODULE ("dma_control_block");


CDMAControlBlock::CDMAControlBlock ()
:	m_pControlBlockBuffer (0),
	m_pControlBlock (0),
	m_pChainedControlBlock (0)
{
	m_pControlBlockBuffer = new (HEAP_DMA30) u8[sizeof (TDMAControlBlock) + 31];
	assert (m_pControlBlockBuffer != 0);

	m_pControlBlock = (TDMAControlBlock *) (((uintptr) m_pControlBlockBuffer + 31) & ~31);
	m_pControlBlock->nReserved[0] = 0;
	m_pControlBlock->nReserved[1] = 0;
}
	

CDMAControlBlock::~CDMAControlBlock (void)
{
	m_pControlBlock = 0;

	delete [] m_pControlBlockBuffer;
	m_pControlBlockBuffer = 0;
}

void CDMAControlBlock::SetupMemCopy (void *pDestination, const void *pSource, size_t nLength,
				unsigned nBurstLength, boolean bCached)
{
	assert (pDestination != 0);
	assert (pSource != 0);
	assert (nLength > 0);
	assert (nBurstLength <= 15);
	assert (nLength <= TXFR_LEN_MAX);

	assert (m_pControlBlock != 0);

	m_pControlBlock->nTransferInformation     =   (nBurstLength << TI_BURST_LENGTH_SHIFT)
												| TI_SRC_WIDTH
												| TI_SRC_INC
												| TI_DEST_WIDTH
												| TI_DEST_INC;
	m_pControlBlock->nSourceAddress           = BUS_ADDRESS ((uintptr) pSource);
	m_pControlBlock->nDestinationAddress      = BUS_ADDRESS ((uintptr) pDestination);
	m_pControlBlock->nTransferLength          = nLength;
	m_pControlBlock->n2DModeStride            = 0;
	m_pControlBlock->nNextControlBlockAddress = 0;

	if (bCached)
	{
		CleanAndInvalidateDataCacheRange ((uintptr) pSource, nLength);
		CleanAndInvalidateDataCacheRange ((uintptr) pDestination, nLength);
	}
	else
	{
		// m_nDestinationAddress = 0;
	}
}

void CDMAControlBlock::SetupIORead (void *pDestination, u32 nIOAddress, size_t nLength, TDREQ DREQ)
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
	m_pControlBlock->nDestinationAddress      = BUS_ADDRESS ((uintptr) pDestination);
	m_pControlBlock->nTransferLength          = nLength;
	m_pControlBlock->n2DModeStride            = 0;
	m_pControlBlock->nNextControlBlockAddress = 0;

	CleanAndInvalidateDataCacheRange ((uintptr) pDestination, nLength);
}

void CDMAControlBlock::SetupIOWrite (u32 nIOAddress, const void *pSource, size_t nLength, TDREQ DREQ)
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
	m_pControlBlock->nSourceAddress           = BUS_ADDRESS ((uintptr) pSource);
	m_pControlBlock->nDestinationAddress      = nIOAddress;
	m_pControlBlock->nTransferLength          = nLength;
	m_pControlBlock->n2DModeStride            = 0;
	m_pControlBlock->nNextControlBlockAddress = 0;

	CleanAndInvalidateDataCacheRange ((uintptr) pSource, nLength);
}

void CDMAControlBlock::SetupMemCopy2D (void *pDestination, const void *pSource,
				  size_t nBlockLength, unsigned nBlockCount, size_t nBlockStride,
				  unsigned nBurstLength)
{
	assert (pDestination != 0);
	assert (pSource != 0);
	assert (nBlockLength > 0);
	assert (nBlockLength <= 0xFFFF);
	assert (nBlockCount > 0);
	assert (nBlockCount <= 0x3FFF);
	assert (nBlockStride <= 0xFFFF);
	assert (nBurstLength <= 15);

	assert (m_pControlBlock != 0);

	m_pControlBlock->nTransferInformation     =   (nBurstLength << TI_BURST_LENGTH_SHIFT)
						    | TI_SRC_WIDTH
						    | TI_SRC_INC
						    | TI_DEST_WIDTH
						    | TI_DEST_INC
						    | TI_TDMODE;						    
	m_pControlBlock->nSourceAddress           = BUS_ADDRESS ((uintptr) pSource);
	m_pControlBlock->nDestinationAddress      = BUS_ADDRESS ((uintptr) pDestination);
	m_pControlBlock->nTransferLength          =   ((nBlockCount-1) << TXFR_LEN_YLENGTH_SHIFT)
						    | (nBlockLength << TXFR_LEN_XLENGTH_SHIFT);
	m_pControlBlock->n2DModeStride            = nBlockStride << STRIDE_DEST_SHIFT;
	m_pControlBlock->nNextControlBlockAddress = 0;

	CleanAndInvalidateDataCacheRange ((uintptr) pSource, nBlockLength*nBlockCount);
}

void *CDMAControlBlock::GetSourceAddress(boolean bAsBusAddress);
void CDMAControlBlock::SetSourceAddress(void *pSource);

void *CDMAControlBlock::GetDestAddress(boolean bAsBusAddress);
void CDMAControlBlock::SetDestAddress(void *pDest);

size_t CDMAControlBlock::GetTransferLength(void);
void CDMAControlBlock::SetTransferLength(size_t nLength);

TDREQ CDMAControlBlock::GetDREQ(void);
void CDMAControlBlock::SetDREQ(TDREQ DREQ);

boolean CDMAControlBlock::GetUseDREQForSrc(void);
void CDMAControlBlock::SetUseDREQForSrc(boolean bUseDREQForSrc);

boolean CDMAControlBlock::GetUseDREQForDest(void);
void CDMAControlBlock::SetUseDREQForDest(boolean bUseDREQForDest);

unsigned CDMAControlBlock::GetBurstLength(void);
void CDMAControlBlock::etBurstLength(unsigned nBurstLength);

unsigned CDMAControlBlock::GetSrcWidth(void);
void CDMAControlBlock::SetSrcWidth(TDMADataWidth srcWidth);

boolean CDMAControlBlock::GetSrcIncrement(void);
void CDMAControlBlock::SetSrcIncrement(boolean bSrcIncrement);

unsigned CDMAControlBlock::GetDestWidth(void);
void CDMAControlBlock::SetDestWidth(TDMADataWidth srcWidth);

boolean CDMAControlBlock::GetDestIncrement(void);
void CDMAControlBlock::SetDestIncrement(boolean bSrcIncrement);

boolean CDMAControlBlock::GetWaitForWriteResponse(void);
void CDMAControlBlock::etWaitForWriteResponse(boolean bWaitForWriteResponse);

unsigned CDMAControlBlock::GetWaitCycles(void);
void CDMAControlBlock::SetWaitCycles(unsigned nWaitCycles);

CDMAControlBlock *CDMAControlBlock::GetChainedControlBlock(void);
void CDMAControlBlock::SetChainedControlBlock(CDMAControlBlock *pControlBlock);

TDMAControlBlock *CDMAControlBlock::GetRawControlBlock(void) 
{
	return m_pControlBlock;
}

