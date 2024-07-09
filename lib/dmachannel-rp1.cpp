//
// dmachannel-rp1.cpp
//
// Portions taken from the Linux driver:
//	drivers/dma/dw-axi-dmac/dw-axi-dmac-platform.c
//	(C) 2017-2018 Synopsys, Inc. (www.synopsys.com)
//	Synopsys DesignWare AXI DMA Controller driver.
//	Author: Eugeniy Paltsev <Eugeniy.Paltsev@synopsys.com>
//	SPDX-License-Identifier: GPL-2.0
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2024  R. Stange <rsta2@o2online.de>
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
#include <circle/dmachannel-rp1.h>
#include <circle/bcm2712.h>
#include <circle/memio.h>
#include <circle/rp1int.h>
#include <circle/bcmpciehostbridge.h>
#include <circle/synchronize.h>
#include <circle/timer.h>
#include <circle/util.h>
#include <circle/logger.h>
#include <circle/debug.h>
#include <assert.h>

#define BLOCK_SIZE		0x40000			// maximum
#define DATA_WIDTH		2			// 4 does not work with mem-copy
#define AXI_MAX_BURST_LEN	DWAXIDMAC_ARWLEN_8
#define PRIORITY(chan)		(chan)

// DMAC device registers
#define COMMON_REG_LEN		0x100
#define CHAN_REG_LEN		0x100

/* Common registers offset */
#define DMAC_ID			0x000 /* R DMAC ID */
#define DMAC_COMPVER		0x008 /* R DMAC Component Version */
#define DMAC_CFG		0x010 /* R/W DMAC Configuration */
#define DMAC_CHEN		0x018 /* R/W DMAC Channel Enable */
#define DMAC_CHEN_L		0x018 /* R/W DMAC Channel Enable 00-31 */
#define DMAC_CHEN_H		0x01C /* R/W DMAC Channel Enable 32-63 */
#define DMAC_CHSUSPREG		0x020 /* R/W DMAC Channel Suspend */
#define DMAC_CHABORTREG		0x028 /* R/W DMAC Channel Abort */
#define DMAC_INTSTATUS		0x030 /* R DMAC Interrupt Status */
#define DMAC_COMMON_INTCLEAR	0x038 /* W DMAC Interrupt Clear */
#define DMAC_COMMON_INTSTATUS_ENA 0x040 /* R DMAC Interrupt Status Enable */
#define DMAC_COMMON_INTSIGNAL_ENA 0x048 /* R/W DMAC Interrupt Signal Enable */
#define DMAC_COMMON_INTSTATUS	0x050 /* R DMAC Interrupt Status */
#define DMAC_RESET		0x058 /* R DMAC Reset Register1 */

/* DMA channel registers offset */
#define CH_SAR			0x000 /* R/W Chan Source Address */
#define CH_DAR			0x008 /* R/W Chan Destination Address */
#define CH_BLOCK_TS		0x010 /* R/W Chan Block Transfer Size */
#define CH_CTL			0x018 /* R/W Chan Control */
#define CH_CTL_L		0x018 /* R/W Chan Control 00-31 */
#define CH_CTL_H		0x01C /* R/W Chan Control 32-63 */
#define CH_CFG			0x020 /* R/W Chan Configuration */
#define CH_CFG_L		0x020 /* R/W Chan Configuration 00-31 */
#define CH_CFG_H		0x024 /* R/W Chan Configuration 32-63 */
#define CH_LLP			0x028 /* R/W Chan Linked List Pointer */
#define CH_LLP_L		0x028 /* R/W Chan Linked List Pointer 00-31 */
#define CH_LLP_H		0x02C /* R/W Chan Linked List Pointer 32-63 */
#define CH_STATUS		0x030 /* R Chan Status */
#define CH_SWHSSRC		0x038 /* R/W Chan SW Handshake Source */
#define CH_SWHSDST		0x040 /* R/W Chan SW Handshake Destination */
#define CH_BLK_TFR_RESUMEREQ	0x048 /* W Chan Block Transfer Resume Req */
#define CH_AXI_ID		0x050 /* R/W Chan AXI ID */
#define CH_AXI_QOS		0x058 /* R/W Chan AXI QOS */
#define CH_SSTAT		0x060 /* R Chan Source Status */
#define CH_DSTAT		0x068 /* R Chan Destination Status */
#define CH_SSTATAR		0x070 /* R/W Chan Source Status Fetch Addr */
#define CH_DSTATAR		0x078 /* R/W Chan Destination Status Fetch Addr */
#define CH_INTSTATUS_ENA	0x080 /* R/W Chan Interrupt Status Enable */
#define CH_INTSTATUS		0x088 /* R/W Chan Interrupt Status */
#define CH_INTSIGNAL_ENA	0x090 /* R/W Chan Interrupt Signal Enable */
#define CH_INTCLEAR		0x098 /* W Chan Interrupt Clear */

/* DMAC_CFG */
#define DMAC_EN_POS			0
#define DMAC_EN_MASK			BIT(DMAC_EN_POS)

#define INT_EN_POS			1
#define INT_EN_MASK			BIT(INT_EN_POS)

/* DMAC_CHEN */
#define DMAC_CHAN_EN_SHIFT		0
#define DMAC_CHAN_EN_WE_SHIFT		8

#define DMAC_CHAN_SUSP_SHIFT		16
#define DMAC_CHAN_SUSP_WE_SHIFT		24

/* DMAC_CHEN2 */
#define DMAC_CHAN_EN2_WE_SHIFT		16

/* DMAC_CHSUSP */
#define DMAC_CHAN_SUSP2_SHIFT		0
#define DMAC_CHAN_SUSP2_WE_SHIFT	16

/* CH_CTL_H */
#define CH_CTL_H_ARLEN_EN		BIT(6)
#define CH_CTL_H_ARLEN_POS		7
#define CH_CTL_H_AWLEN_EN		BIT(15)
#define CH_CTL_H_AWLEN_POS		16

enum {
	DWAXIDMAC_ARWLEN_1		= 0,
	DWAXIDMAC_ARWLEN_2		= 1,
	DWAXIDMAC_ARWLEN_4		= 3,
	DWAXIDMAC_ARWLEN_8		= 7,
	DWAXIDMAC_ARWLEN_16		= 15,
	DWAXIDMAC_ARWLEN_32		= 31,
	DWAXIDMAC_ARWLEN_64		= 63,
	DWAXIDMAC_ARWLEN_128		= 127,
	DWAXIDMAC_ARWLEN_256		= 255,
	DWAXIDMAC_ARWLEN_MIN		= DWAXIDMAC_ARWLEN_1,
	DWAXIDMAC_ARWLEN_MAX		= DWAXIDMAC_ARWLEN_256
};

#define CH_CTL_H_LLI_LAST		BIT(30)
#define CH_CTL_H_LLI_VALID		BIT(31)

/* CH_CTL_L */
#define CH_CTL_L_LAST_WRITE_EN		BIT(30)

#define CH_CTL_L_DST_MSIZE_POS		18
#define CH_CTL_L_SRC_MSIZE_POS		14

enum {
	DWAXIDMAC_BURST_TRANS_LEN_1	= 0,
	DWAXIDMAC_BURST_TRANS_LEN_4,
	DWAXIDMAC_BURST_TRANS_LEN_8,
	DWAXIDMAC_BURST_TRANS_LEN_16,
	DWAXIDMAC_BURST_TRANS_LEN_32,
	DWAXIDMAC_BURST_TRANS_LEN_64,
	DWAXIDMAC_BURST_TRANS_LEN_128,
	DWAXIDMAC_BURST_TRANS_LEN_256,
	DWAXIDMAC_BURST_TRANS_LEN_512,
	DWAXIDMAC_BURST_TRANS_LEN_1024
};

#define CH_CTL_L_DST_WIDTH_POS		11
#define CH_CTL_L_SRC_WIDTH_POS		8

#define CH_CTL_L_DST_INC_POS		6
#define CH_CTL_L_SRC_INC_POS		4
enum {
	DWAXIDMAC_CH_CTL_L_INC		= 0,
	DWAXIDMAC_CH_CTL_L_NOINC
};

#define CH_CTL_L_DST_MAST		BIT(2)
#define CH_CTL_L_SRC_MAST		BIT(0)

/* CH_CFG_H */
#define CH_CFG_H_PRIORITY_POS		17
#define CH_CFG_H_DST_PER_POS		12
#define CH_CFG_H_SRC_PER_POS		7
#define CH_CFG_H_HS_SEL_DST_POS		4
#define CH_CFG_H_HS_SEL_SRC_POS		3
enum {
	DWAXIDMAC_HS_SEL_HW		= 0,
	DWAXIDMAC_HS_SEL_SW
};

#define CH_CFG_H_TT_FC_POS		0
enum {
	DWAXIDMAC_TT_FC_MEM_TO_MEM_DMAC	= 0,
	DWAXIDMAC_TT_FC_MEM_TO_PER_DMAC,
	DWAXIDMAC_TT_FC_PER_TO_MEM_DMAC,
	DWAXIDMAC_TT_FC_PER_TO_PER_DMAC,
	DWAXIDMAC_TT_FC_PER_TO_MEM_SRC,
	DWAXIDMAC_TT_FC_PER_TO_PER_SRC,
	DWAXIDMAC_TT_FC_MEM_TO_PER_DST,
	DWAXIDMAC_TT_FC_PER_TO_PER_DST
};

/* CH_CFG_L */
#define CH_CFG_L_DST_MULTBLK_TYPE_POS	2
#define CH_CFG_L_SRC_MULTBLK_TYPE_POS	0
enum {
	DWAXIDMAC_MBLK_TYPE_CONTIGUOUS	= 0,
	DWAXIDMAC_MBLK_TYPE_RELOAD,
	DWAXIDMAC_MBLK_TYPE_SHADOW_REG,
	DWAXIDMAC_MBLK_TYPE_LL
};

/* CH_CFG2 */
#define CH_CFG2_L_SRC_PER_POS		4
#define CH_CFG2_L_DST_PER_POS		11

#define CH_CFG2_H_TT_FC_POS		0
#define CH_CFG2_H_HS_SEL_SRC_POS	3
#define CH_CFG2_H_HS_SEL_DST_POS	4
#define CH_CFG2_H_PRIORITY_POS		20

#define GENMASK(h, l)			(((~0UL) - (1UL << (l)) + 1) &	\
					(~0UL >> (64 - 1 - (h))))
enum {
	DWAXIDMAC_IRQ_NONE		= 0,
	DWAXIDMAC_IRQ_BLOCK_TRF		= BIT(0),
	DWAXIDMAC_IRQ_DMA_TRF		= BIT(1),
	DWAXIDMAC_IRQ_SRC_TRAN		= BIT(3),
	DWAXIDMAC_IRQ_DST_TRAN		= BIT(4),
	DWAXIDMAC_IRQ_SRC_DEC_ERR	= BIT(5),
	DWAXIDMAC_IRQ_DST_DEC_ERR	= BIT(6),
	DWAXIDMAC_IRQ_SRC_SLV_ERR	= BIT(7),
	DWAXIDMAC_IRQ_DST_SLV_ERR	= BIT(8),
	DWAXIDMAC_IRQ_LLI_RD_DEC_ERR	= BIT(9),
	DWAXIDMAC_IRQ_LLI_WR_DEC_ERR	= BIT(10),
	DWAXIDMAC_IRQ_LLI_RD_SLV_ERR	= BIT(11),
	DWAXIDMAC_IRQ_LLI_WR_SLV_ERR	= BIT(12),
	DWAXIDMAC_IRQ_INVALID_ERR	= BIT(13),
	DWAXIDMAC_IRQ_MULTIBLKTYPE_ERR	= BIT(14),
	DWAXIDMAC_IRQ_DEC_ERR		= BIT(16),
	DWAXIDMAC_IRQ_WR2RO_ERR		= BIT(17),
	DWAXIDMAC_IRQ_RD2RWO_ERR	= BIT(18),
	DWAXIDMAC_IRQ_WRONCHEN_ERR	= BIT(19),
	DWAXIDMAC_IRQ_SHADOWREG_ERR	= BIT(20),
	DWAXIDMAC_IRQ_WRONHOLD_ERR	= BIT(21),
	DWAXIDMAC_IRQ_LOCK_CLEARED	= BIT(27),
	DWAXIDMAC_IRQ_SRC_SUSPENDED	= BIT(28),
	DWAXIDMAC_IRQ_SUSPENDED		= BIT(29),
	DWAXIDMAC_IRQ_DISABLED		= BIT(30),
	DWAXIDMAC_IRQ_ABORTED		= BIT(31),
	DWAXIDMAC_IRQ_ALL_ERR		= (GENMASK(21, 16) | GENMASK(14, 5)),
	DWAXIDMAC_IRQ_ALL		= GENMASK(31, 0)
};

enum {
	DWAXIDMAC_TRANS_WIDTH_8		= 0,
	DWAXIDMAC_TRANS_WIDTH_16,
	DWAXIDMAC_TRANS_WIDTH_32,
	DWAXIDMAC_TRANS_WIDTH_64,
	DWAXIDMAC_TRANS_WIDTH_128,
	DWAXIDMAC_TRANS_WIDTH_256,
	DWAXIDMAC_TRANS_WIDTH_512,
	DWAXIDMAC_TRANS_WIDTH_MAX	= DWAXIDMAC_TRANS_WIDTH_512
};

// macros
#define ARM_DMA_RP1_CHAN(chan)	(ARM_DMA_RP1_BASE + COMMON_REG_LEN + (chan) * CHAN_REG_LEN)

#define PTR_TO_DMA(ptr)		(  reinterpret_cast<uintptr> (ptr)	\
				 | CBcmPCIeHostBridge::GetDMAAddress ())
#define DMA_TO_VIRT(dma)	((dma) & 0x3FFFFFFFFUL)

#define VIRT_TO_RP1_BUS(virt)	(0xC040000000UL | ((virt) & 0x3FFFFFFFUL))

#define DIV_ROUND_UP(n, m)	(((n) + (m) - 1) / (m))

#define lower_32_bits(v)	((v) & 0xFFFFFFFFU)
#define upper_32_bits(v)	((v) >> 32)

#define ffs			__builtin_ffs

LOGMODULE ("dma-rp1");

unsigned CDMAChannelRP1::s_nInstanceCount = 0;

CInterruptSystem *CDMAChannelRP1::s_pInterruptSystem = nullptr;

CSpinLock CDMAChannelRP1::s_SpinLock;

CDMAChannelRP1 *CDMAChannelRP1::s_pThis[Channels] = {nullptr};

CDMAChannelRP1::CDMAChannelRP1 (unsigned nChannel, CInterruptSystem *pInterruptSystem)
:	m_nChannel {nChannel},
	m_nBuffers {0},
	m_Direction {DirectionUnknown},
	m_DREQ {DREQSourceNone},
	m_pCompletionRoutine {nullptr}
{
	assert (m_nChannel < Channels);
	s_pThis[m_nChannel] = this;

	for (int i = 0; i < MaxCyclicBuffers; i++)
	{
		m_pLLI[i] = new TLinkedListItem;
		assert (m_pLLI[i]);
	}

	if (s_nInstanceCount++)
	{
		return;
	}

	s_pInterruptSystem = pInterruptSystem;
	assert (s_pInterruptSystem);
	s_pInterruptSystem->ConnectIRQ (RP1_IRQ_DMA, InterruptHandler, nullptr);

	// reset controller
	write32 (ARM_DMA_RP1_BASE + DMAC_RESET, 1);
	for (int retries = 1000; read32 (ARM_DMA_RP1_BASE + DMAC_RESET); retries--)
	{
		if (!retries)
		{
			LOGERR ("Reset failed");

			return;
		}

		CTimer::SimpleusDelay (1);
	}

	u32 val;
	for (int i = 0; i < Channels; i++)
	{
		// disable IRQ
		write32 (ARM_DMA_RP1_CHAN (i) + CH_INTSTATUS_ENA, DWAXIDMAC_IRQ_NONE);

		// disable channel
		val = read32 (ARM_DMA_RP1_BASE + DMAC_CHEN);
		val &= ~(BIT (i) << DMAC_CHAN_EN_SHIFT);
		val |= BIT (i) << DMAC_CHAN_EN_WE_SHIFT;
		write32 (ARM_DMA_RP1_BASE + DMAC_CHEN, val);
	}

	// enable DMA
	val = read32 (ARM_DMA_RP1_BASE + DMAC_CFG);
	val |= DMAC_EN_MASK;
	write32 (ARM_DMA_RP1_BASE + DMAC_CFG, val);

	// enable IRQ
	val = read32 (ARM_DMA_RP1_BASE + DMAC_CFG);
	val |= INT_EN_MASK;
	write32 (ARM_DMA_RP1_BASE + DMAC_CFG, val);
}

CDMAChannelRP1::~CDMAChannelRP1 (void)
{
	assert (m_nChannel < Channels);

	if (!--s_nInstanceCount)
	{
		// disable IRQ
		u32 val = read32 (ARM_DMA_RP1_BASE + DMAC_CFG);
		val &= ~INT_EN_MASK;
		write32 (ARM_DMA_RP1_BASE + DMAC_CFG, val);

		for (int i = 0; i < Channels; i++)
		{
			// disable channel
			val = read32 (ARM_DMA_RP1_BASE + DMAC_CHEN);
			val &= ~(BIT (i) << DMAC_CHAN_EN_SHIFT);
			val |= BIT (i) << DMAC_CHAN_EN_WE_SHIFT;
			write32 (ARM_DMA_RP1_BASE + DMAC_CHEN, val);

			// disable IRQ
			write32 (ARM_DMA_RP1_CHAN (i) + CH_INTSTATUS_ENA, DWAXIDMAC_IRQ_NONE);
		}

		// disable DMA
		val = read32 (ARM_DMA_RP1_BASE + DMAC_CFG);
		val &= ~DMAC_EN_MASK;
		write32 (ARM_DMA_RP1_BASE + DMAC_CFG, val);

		assert (s_pInterruptSystem);
		s_pInterruptSystem->DisconnectIRQ (RP1_IRQ_DMA);
		s_pInterruptSystem = nullptr;
	}

	for (int i = 0; i < MaxCyclicBuffers; i++)
	{
		delete m_pLLI[i];
	}

	s_pThis[m_nChannel] = nullptr;
	m_nChannel = Channels;
}

void CDMAChannelRP1::SetupMemCopy (void *pDestination, const void *pSource, size_t ulLength,
				   boolean bCached)
{
	assert (m_nChannel < Channels);
	assert (!(  read32 (ARM_DMA_RP1_BASE + DMAC_CHEN) 		// channel is idle
		  & (BIT (m_nChannel) << DMAC_CHAN_EN_SHIFT)));

	assert (pDestination);
	assert (pSource);
	assert (ulLength);
	assert (ulLength <= BLOCK_SIZE);

	uintptr ulDest = PTR_TO_DMA (pDestination);
	uintptr ulSrc  = PTR_TO_DMA (pSource);

	u32 nWidth = ffs (ulDest | ulSrc | ulLength | BIT (DATA_WIDTH));
	assert (nWidth);

	assert (m_pLLI[0]);
	memset (m_pLLI[0], 0, sizeof *m_pLLI[0]);

	m_pLLI[0]->sar = ulSrc;
	m_pLLI[0]->dar = ulDest;

	m_pLLI[0]->block_ts_lo = (ulLength >> nWidth) - 1;

	m_pLLI[0]->ctl_hi =   CH_CTL_H_LLI_VALID
			    | CH_CTL_H_LLI_LAST
			    | CH_CTL_H_ARLEN_EN
			    | AXI_MAX_BURST_LEN << CH_CTL_H_ARLEN_POS
			    | CH_CTL_H_AWLEN_EN
			    | AXI_MAX_BURST_LEN << CH_CTL_H_AWLEN_POS;

	m_pLLI[0]->ctl_lo =   DWAXIDMAC_BURST_TRANS_LEN_4 << CH_CTL_L_DST_MSIZE_POS
			    | DWAXIDMAC_BURST_TRANS_LEN_4 << CH_CTL_L_SRC_MSIZE_POS
			    | nWidth << CH_CTL_L_DST_WIDTH_POS
			    | nWidth << CH_CTL_L_SRC_WIDTH_POS
			    | DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_DST_INC_POS
			    | DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_SRC_INC_POS;

	// not necessary, because LLI has been zeroed before
	//m_pLLI[0]->ctl_lo &= CH_CTL_L_SRC_MAST | CH_CTL_L_DST_MAST; // source and dest master AXI0
	//m_pLLI[0]->llp = 0;	// LLI fetching master AXI0

	if (bCached)
	{
		InvalidateDataCacheRange (reinterpret_cast<uintptr> (pDestination), ulLength);
		CleanDataCacheRange (reinterpret_cast<uintptr> (pSource), ulLength);
	}

	CleanAndInvalidateDataCacheRange (reinterpret_cast<uintptr> (m_pLLI[0]), sizeof *m_pLLI[0]);

	m_nBuffers = 1;
	m_ulLength = ulLength;
	m_Direction = DirectionMem2Mem;
	m_DREQ = DREQSourceNone;
}

void CDMAChannelRP1::SetupIORead (void *pDestination, uintptr ulIOAddress, size_t ulLength,
				  TDREQ DREQ, unsigned nIORegWidth)
{
	SetupCyclicIORead (&pDestination, ulIOAddress, 1, ulLength, DREQ, nIORegWidth);
}

void CDMAChannelRP1::SetupCyclicIORead (void *ppDestinations[], uintptr ulIOAddress,
					unsigned nBuffers, size_t ulLength, TDREQ DREQ,
					unsigned nIORegWidth)
{
	assert (m_nChannel < Channels);
	assert (!(  read32 (ARM_DMA_RP1_BASE + DMAC_CHEN) 		// channel is idle
		  & (BIT (m_nChannel) << DMAC_CHAN_EN_SHIFT)));

	assert (ulIOAddress);
	assert (ppDestinations);
	assert (ulLength);
	assert (ulLength <= BLOCK_SIZE);

	// convert to bus address
	uintptr ulSrc = VIRT_TO_RP1_BUS (ulIOAddress);

	for (unsigned i = 0; i < nBuffers; i++)
	{
		assert (ppDestinations[i]);
		uintptr ulDest  = PTR_TO_DMA (ppDestinations[i]);
		assert (!(ulDest & 3));

		u32 nMemWidth = ffs (ulDest | ulLength | BIT (DATA_WIDTH));
		assert (nMemWidth);
		if (nMemWidth > DWAXIDMAC_TRANS_WIDTH_32)
		{
			nMemWidth = DWAXIDMAC_TRANS_WIDTH_32;
		}

		assert (nIORegWidth == 1 || nIORegWidth == 4);
		u32 nRegWidth = ffs (nIORegWidth) - 1;

		if (nMemWidth > nRegWidth)
		{
			nMemWidth = nRegWidth;
		}

		assert (m_pLLI[i]);
		memset (m_pLLI[i], 0, sizeof *m_pLLI[i]);

		m_pLLI[i]->sar = ulSrc;
		m_pLLI[i]->dar = ulDest;

		m_pLLI[i]->block_ts_lo = (ulLength >> nRegWidth) - 1;

		m_pLLI[i]->ctl_hi =   CH_CTL_H_LLI_VALID
				    | CH_CTL_H_LLI_LAST
				    | CH_CTL_H_ARLEN_EN
				    | AXI_MAX_BURST_LEN << CH_CTL_H_ARLEN_POS
				    | CH_CTL_H_AWLEN_EN
				    | AXI_MAX_BURST_LEN << CH_CTL_H_AWLEN_POS;

		m_pLLI[i]->ctl_lo =   DWAXIDMAC_BURST_TRANS_LEN_4 << CH_CTL_L_DST_MSIZE_POS
				    | DWAXIDMAC_BURST_TRANS_LEN_1 << CH_CTL_L_SRC_MSIZE_POS
				    | nMemWidth << CH_CTL_L_DST_WIDTH_POS
				    | nRegWidth << CH_CTL_L_SRC_WIDTH_POS
				    | DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_DST_INC_POS
				    | DWAXIDMAC_CH_CTL_L_NOINC << CH_CTL_L_SRC_INC_POS;

		// not necessary, because LLI has been zeroed before
		//m_pLLI[i]->ctl_lo &= CH_CTL_L_SRC_MAST;		// source master AXI0

		if (nBuffers > 1)
		{
			m_pLLI[i]->llp = PTR_TO_DMA (m_pLLI[i == nBuffers-1 ? 0 : i+1]);
		}

		// m_pLLI[i]->llp |= 0;		// LLI fetching master AXI0

		InvalidateDataCacheRange (reinterpret_cast<uintptr> (ppDestinations[i]), ulLength);

		CleanAndInvalidateDataCacheRange (reinterpret_cast<uintptr> (m_pLLI[i]),
						  sizeof *m_pLLI[i]);
	}

	m_nBuffers = nBuffers;
	m_ulLength = ulLength;
	m_Direction = DirectionDev2Mem;
	m_DREQ = DREQ;
}

void CDMAChannelRP1::SetupIOWrite (uintptr ulIOAddress, const void *pSource, size_t ulLength,
				   TDREQ DREQ, unsigned nIORegWidth)
{
	SetupCyclicIOWrite (ulIOAddress, &pSource, 1, ulLength, DREQ, nIORegWidth);
}

void CDMAChannelRP1::SetupCyclicIOWrite (uintptr ulIOAddress, const void *ppSources[],
					 unsigned nBuffers, size_t ulLength, TDREQ DREQ,
					 unsigned nIORegWidth)
{
	assert (m_nChannel < Channels);
	assert (!(  read32 (ARM_DMA_RP1_BASE + DMAC_CHEN) 		// channel is idle
		  & (BIT (m_nChannel) << DMAC_CHAN_EN_SHIFT)));

	assert (ulIOAddress);
	assert (ppSources);
	assert (ulLength);
	assert (ulLength <= BLOCK_SIZE);

	// convert to bus address
	uintptr ulDest = VIRT_TO_RP1_BUS (ulIOAddress);

	for (unsigned i = 0; i < nBuffers; i++)
	{
		assert (ppSources[i]);
		uintptr ulSrc  = PTR_TO_DMA (ppSources[i]);
		assert (!(ulSrc & 3));

		u32 nMemWidth = ffs (ulSrc | ulLength | BIT (DATA_WIDTH));
		assert (nMemWidth);
		if (nMemWidth > DWAXIDMAC_TRANS_WIDTH_32)
		{
			nMemWidth = DWAXIDMAC_TRANS_WIDTH_32;
		}

		assert (nIORegWidth == 1 || nIORegWidth == 4);
		u32 nRegWidth = ffs (nIORegWidth) - 1;

		assert (m_pLLI[i]);
		memset (m_pLLI[i], 0, sizeof *m_pLLI[i]);

		m_pLLI[i]->sar = ulSrc;
		m_pLLI[i]->dar = ulDest;

		m_pLLI[i]->block_ts_lo = (ulLength >> nMemWidth) - 1;

		m_pLLI[i]->ctl_hi =   CH_CTL_H_LLI_VALID
				    | CH_CTL_H_LLI_LAST
				    | CH_CTL_H_ARLEN_EN
				    | AXI_MAX_BURST_LEN << CH_CTL_H_ARLEN_POS
				    | CH_CTL_H_AWLEN_EN
				    | AXI_MAX_BURST_LEN << CH_CTL_H_AWLEN_POS;

		m_pLLI[i]->ctl_lo =   DWAXIDMAC_BURST_TRANS_LEN_1 << CH_CTL_L_DST_MSIZE_POS
				    | DWAXIDMAC_BURST_TRANS_LEN_4 << CH_CTL_L_SRC_MSIZE_POS
				    | nRegWidth << CH_CTL_L_DST_WIDTH_POS
				    | nMemWidth << CH_CTL_L_SRC_WIDTH_POS
				    | DWAXIDMAC_CH_CTL_L_NOINC << CH_CTL_L_DST_INC_POS
				    | DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_SRC_INC_POS;

		// not necessary, because LLI has been zeroed before
		//m_pLLI[i]->ctl_lo &= CH_CTL_L_SRC_MAST;		// source master AXI0

		if (nBuffers > 1)
		{
			m_pLLI[i]->llp = PTR_TO_DMA (m_pLLI[i == nBuffers-1 ? 0 : i+1]);
		}

		// m_pLLI[i]->llp |= 0;		// LLI fetching master AXI0

		CleanDataCacheRange (reinterpret_cast<uintptr> (ppSources[i]), ulLength);

		CleanAndInvalidateDataCacheRange (reinterpret_cast<uintptr> (m_pLLI[i]),
						  sizeof *m_pLLI[i]);
	}

	m_nBuffers = nBuffers;
	m_ulLength = ulLength;
	m_Direction = DirectionMem2Dev;
	m_DREQ = DREQ;
}

void CDMAChannelRP1::SetCompletionRoutine (TCompletionRoutine *pRoutine, void *pParam)
{
	assert (pRoutine);

	m_pCompletionRoutine = pRoutine;
	m_pCompletionParam = pParam;
}

void CDMAChannelRP1::Start (void)
{
	assert (m_nChannel < Channels);
	assert (!(  read32 (ARM_DMA_RP1_BASE + DMAC_CHEN) 		// channel is idle
		  & (BIT (m_nChannel) << DMAC_CHAN_EN_SHIFT)));
	assert (read32 (ARM_DMA_RP1_BASE + DMAC_CFG) & DMAC_EN_MASK);	// controller is enabled

	u8 tt_fc = DWAXIDMAC_TT_FC_MEM_TO_MEM_DMAC;
	u8 dst_per = 0;
	u8 src_per = 0;

	switch (m_Direction)
	{
	case DirectionMem2Mem:
		break;

	case DirectionMem2Dev:
		tt_fc = DWAXIDMAC_TT_FC_MEM_TO_PER_DMAC;
		dst_per = m_DREQ;
		break;

	case DirectionDev2Mem:
		tt_fc = DWAXIDMAC_TT_FC_PER_TO_MEM_DMAC;
		src_per = m_DREQ;
		break;

	default:
		assert (0);
		break;
	}

	write32 (ARM_DMA_RP1_CHAN (m_nChannel) + CH_CFG_L,
		   DWAXIDMAC_MBLK_TYPE_LL << CH_CFG_L_DST_MULTBLK_TYPE_POS
		 | DWAXIDMAC_MBLK_TYPE_LL<< CH_CFG_L_SRC_MULTBLK_TYPE_POS
		 | src_per << CH_CFG2_L_SRC_PER_POS
		 | dst_per << CH_CFG2_L_DST_PER_POS);

	write32 (ARM_DMA_RP1_CHAN (m_nChannel) + CH_CFG_H,
		   tt_fc << CH_CFG2_H_TT_FC_POS
		 | DWAXIDMAC_HS_SEL_HW << CH_CFG2_H_HS_SEL_SRC_POS
		 | DWAXIDMAC_HS_SEL_HW << CH_CFG2_H_HS_SEL_DST_POS
		 | PRIORITY (m_nChannel) << CH_CFG2_H_PRIORITY_POS);

	m_nCurrentBuffer = 0;

	assert (m_pLLI[0]);
	u64 llp = PTR_TO_DMA (m_pLLI[0]);
	//llp |= 0;				// LLI fetching master AXI0
	write32 (ARM_DMA_RP1_CHAN (m_nChannel) + CH_LLP_L, lower_32_bits (llp));
	write32 (ARM_DMA_RP1_CHAN (m_nChannel) + CH_LLP_H, upper_32_bits (llp));

	u32 nIntMask = DWAXIDMAC_IRQ_DMA_TRF | DWAXIDMAC_IRQ_ALL_ERR;
	write32 (ARM_DMA_RP1_CHAN (m_nChannel) + CH_INTSIGNAL_ENA, nIntMask);

	// generate 'suspend' status but don't generate interrupt
	nIntMask |= DWAXIDMAC_IRQ_SUSPENDED;
	write32 (ARM_DMA_RP1_CHAN (m_nChannel) + CH_INTSTATUS_ENA, nIntMask);

	s_SpinLock.Acquire ();
	write32 (ARM_DMA_RP1_BASE + DMAC_CHEN,   read32 (ARM_DMA_RP1_BASE + DMAC_CHEN)
					       | BIT (m_nChannel) << DMAC_CHAN_EN_SHIFT
					       | BIT (m_nChannel) << DMAC_CHAN_EN_WE_SHIFT);
	s_SpinLock.Release ();
}

void CDMAChannelRP1::Cancel (void)
{
	assert (m_nChannel < Channels);

	// disable channel
	s_SpinLock.Acquire ();
	u32 val = read32 (ARM_DMA_RP1_BASE + DMAC_CHEN);
	val &= ~(BIT (m_nChannel) << DMAC_CHAN_EN_SHIFT);
	val |=   BIT (m_nChannel) << DMAC_CHAN_EN_WE_SHIFT;
	write32 (ARM_DMA_RP1_BASE + DMAC_CHEN, val);
	s_SpinLock.Release ();

	u32 chan_active = BIT (m_nChannel) << DMAC_CHAN_EN_SHIFT;
	for (unsigned retries = 50; read32 (ARM_DMA_RP1_BASE + DMAC_CHEN) & chan_active; retries--)
	{
		if (!retries)
		{
			LOGWARN ("Channel failed to stop (%u)", m_nChannel);

			break;
		}

		CTimer::SimpleusDelay (1000);
	}
}

void CDMAChannelRP1::ChannelInterruptHandler (boolean bStatus)
{
	assert (m_pCompletionRoutine);
	(*m_pCompletionRoutine) (m_nChannel, m_nCurrentBuffer, bStatus, m_pCompletionParam);

	if (   bStatus
	    && m_nBuffers > 1)
	{
		assert (m_nCurrentBuffer < m_nBuffers);
		TLinkedListItem *pLLI = m_pLLI[m_nCurrentBuffer];
		assert (pLLI);

		if (m_Direction == DirectionMem2Dev)
		{
			CleanDataCacheRange (DMA_TO_VIRT (pLLI->sar), m_ulLength);
		}
		else
		{
			assert (m_Direction == DirectionDev2Mem);
			InvalidateDataCacheRange (DMA_TO_VIRT (pLLI->dar), m_ulLength);
		}

		write32 (ARM_DMA_RP1_CHAN (m_nChannel) + CH_INTCLEAR, pLLI->status_lo);

		pLLI->ctl_hi |= CH_CTL_H_LLI_VALID;

		CleanAndInvalidateDataCacheRange (reinterpret_cast<uintptr> (pLLI), sizeof *pLLI);

		if (++m_nCurrentBuffer == m_nBuffers)
		{
			m_nCurrentBuffer = 0;
		}
	}
}

void CDMAChannelRP1::InterruptHandler (void *pParam)
{
	s_SpinLock.Acquire ();

	for (int i = 0; i < Channels; i++)
	{
		CDMAChannelRP1 *pThis = s_pThis[i];
		if (!pThis)
		{
			continue;
		}

		u32 nStatus = read32 (ARM_DMA_RP1_CHAN (i) + CH_INTSTATUS);
		write32 (ARM_DMA_RP1_CHAN (i) + CH_INTCLEAR, nStatus);

		// if (nStatus) LOGDBG ("IRQ status is 0x%08X (chan %d)", nStatus, i);

		if (nStatus & DWAXIDMAC_IRQ_ALL_ERR)
		{
			// disable channel
			u32 val = read32 (ARM_DMA_RP1_BASE + DMAC_CHEN);
			val &= ~(BIT (i) << DMAC_CHAN_EN_SHIFT);
			val |= BIT (i) << DMAC_CHAN_EN_WE_SHIFT;
			write32 (ARM_DMA_RP1_BASE + DMAC_CHEN, val);

			LOGWARN ("Transfer failed (chan %d, buf %u, stat 0x%X)",
				 i, pThis->m_nCurrentBuffer, nStatus);

#ifndef NDEBUG
			pThis->DumpStatus ();
#endif

			s_SpinLock.Release ();

			pThis->ChannelInterruptHandler (FALSE);

			s_SpinLock.Acquire ();
		}
		else if (nStatus & DWAXIDMAC_IRQ_DMA_TRF)
		{
			u32 val = read32 (ARM_DMA_RP1_BASE + DMAC_CHEN);
			assert (!(val & (BIT (i) << DMAC_CHAN_EN_SHIFT))); // channel is disabled

			if (pThis->m_nBuffers > 1)
			{
				// enable cyclic channel again
				write32 (ARM_DMA_RP1_BASE + DMAC_CHEN,
					   val
					 | BIT (i) << DMAC_CHAN_EN_SHIFT
					 | BIT (i) << DMAC_CHAN_EN_WE_SHIFT);
			}

			s_SpinLock.Release ();

			pThis->ChannelInterruptHandler (TRUE);

			s_SpinLock.Acquire ();
		}
	}

	s_SpinLock.Release ();
}

#ifndef NDEBUG

void CDMAChannelRP1::DumpStatus (void)
{
	LOGDBG ("COMMON");
	for (uintptr addr = ARM_DMA_RP1_BASE; addr <= ARM_DMA_RP1_BASE + DMAC_CHEN_H; addr += 4)
	{
		LOGDBG ("%02X: %08X", addr & 0xFF, read32 (addr));
	}

	assert (m_nChannel < Channels);
	LOGDBG ("CHANNEL %u", m_nChannel);
	for (uintptr addr = ARM_DMA_RP1_CHAN (m_nChannel);
	     addr <= ARM_DMA_RP1_CHAN (m_nChannel) + CH_LLP_H; addr += 4)
	{
		LOGDBG ("%02X: %08X", addr & 0xFF, read32 (addr));
	}

	for (unsigned i = 0; i < m_nBuffers; i++)
	{
		LOGDBG ("LLI %u", i);
		assert (m_pLLI[i]);
		debug_hexdump (m_pLLI[i], sizeof *m_pLLI[i], From);
	}
}

#endif
