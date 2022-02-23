//
/// \file dmacommon.h
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
#ifndef _circle_dmacommon_h
#define _circle_dmacommon_h

#include <circle/bcm2835.h>
#include <circle/types.h>

enum TDREQ
{
	DREQSourceNone	 = 0,
#if RASPPI >= 4
	DREQSourcePWM1	 = 1,
#endif
	DREQSourcePCMTX	 = 2,
	DREQSourcePCMRX	 = 3,
	DREQSourceSMI	 = 4,
	DREQSourcePWM	 = 5,
	DREQSourceSPITX	 = 6,
	DREQSourceSPIRX	 = 7,
#if RASPPI >= 4
	DREQSourceHDMI	 = 10,
#endif
	DREQSourceEMMC	 = 11,
	DREQSourceUARTTX = 12,
	DREQSourceUARTRX = 14,
#if RASPPI <= 3
	DREQSourceHDMI	 = 17
#endif
};

typedef void TDMACompletionRoutine (unsigned nChannel, boolean bStatus, void *pParam);

//
// Legacy platform DMA controller
//
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
	#define TXFR_LEN_MAX_LITE		0xFFFF
#define ARM_DMACHAN_STRIDE(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x18)
	#define STRIDE_SRC_SHIFT		0
	#define STRIDE_DEST_SHIFT		16
#define ARM_DMACHAN_NEXTCONBK(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x1C)
#define ARM_DMACHAN_DEBUG(chan)		(ARM_DMA_BASE + ((chan) * 0x100) + 0x20)
	#define DEBUG_LITE			(1 << 28)
#define ARM_DMA_INT_STATUS		(ARM_DMA_BASE + 0xFE0)
#define ARM_DMA_ENABLE			(ARM_DMA_BASE + 0xFF0)

#endif
