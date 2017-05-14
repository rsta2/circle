//
// pwmsoundbasedevice.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2017  R. Stange <rsta2@o2online.de>
//
// Information to implement PWM sound is from:
//	"Bare metal sound" by Joeboy (RPi forum)
//	"Raspberry Pi Bare Metal Code" by krom (Peter Lemon)
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
#include <circle/pwmsoundbasedevice.h>
#include <circle/bcm2835.h>
#include <circle/bcm2835int.h>
#include <circle/memio.h>
#include <circle/timer.h>
#include <circle/synchronize.h>
#include <circle/util.h>
#include <assert.h>

#define CLOCK_FREQ		500000000		// PLLD
#define CLOCK_DIVIDER		2

//
// PWM control register
//
#define ARM_PWM_CTL_PWEN1	(1 << 0)
#define ARM_PWM_CTL_MODE1	(1 << 1)
#define ARM_PWM_CTL_RPTL1	(1 << 2)
#define ARM_PWM_CTL_SBIT1	(1 << 3)
#define ARM_PWM_CTL_POLA1	(1 << 4)
#define ARM_PWM_CTL_USEF1	(1 << 5)
#define ARM_PWM_CTL_CLRF1	(1 << 6)
#define ARM_PWM_CTL_MSEN1	(1 << 7)
#define ARM_PWM_CTL_PWEN2	(1 << 8)
#define ARM_PWM_CTL_MODE2	(1 << 9)
#define ARM_PWM_CTL_RPTL2	(1 << 10)
#define ARM_PWM_CTL_SBIT2	(1 << 11)
#define ARM_PWM_CTL_POLA2	(1 << 12)
#define ARM_PWM_CTL_USEF2	(1 << 13)
#define ARM_PWM_CTL_MSEN2	(1 << 14)

//
// PWM status register
//
#define ARM_PWM_STA_FULL1	(1 << 0)
#define ARM_PWM_STA_EMPT1	(1 << 1)
#define ARM_PWM_STA_WERR1	(1 << 2)
#define ARM_PWM_STA_RERR1	(1 << 3)
#define ARM_PWM_STA_GAPO1	(1 << 4)
#define ARM_PWM_STA_GAPO2	(1 << 5)
#define ARM_PWM_STA_GAPO3	(1 << 6)
#define ARM_PWM_STA_GAPO4	(1 << 7)
#define ARM_PWM_STA_BERR	(1 << 8)
#define ARM_PWM_STA_STA1	(1 << 9)
#define ARM_PWM_STA_STA2	(1 << 10)
#define ARM_PWM_STA_STA3	(1 << 11)
#define ARM_PWM_STA_STA4	(1 << 12)

//
// PWM DMA configuration register
//
#define ARM_PWM_DMAC_DREQ__SHIFT	0
#define ARM_PWM_DMAC_PANIC__SHIFT	8
#define ARM_PWM_DMAC_ENAB		(1 << 31)

//
// DMA controller
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
#define ARM_DMACHAN_STRIDE(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x18)
	#define STRIDE_SRC_SHIFT		0
	#define STRIDE_DEST_SHIFT		16
#define ARM_DMACHAN_NEXTCONBK(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x1C)
#define ARM_DMACHAN_DEBUG(chan)		(ARM_DMA_BASE + ((chan) * 0x100) + 0x20)
	#define DEBUG_LITE			(1 << 28)
#define ARM_DMA_INT_STATUS		(ARM_DMA_BASE + 0xFE0)
#define ARM_DMA_ENABLE			(ARM_DMA_BASE + 0xFF0)

CPWMSoundBaseDevice::CPWMSoundBaseDevice (CInterruptSystem *pInterrupt,
					  unsigned	    nSampleRate,
					  unsigned	    nChunkSize)
:	m_pInterruptSystem (pInterrupt),
	m_nChunkSize (nChunkSize),
	m_nRange ((CLOCK_FREQ / CLOCK_DIVIDER + nSampleRate/2) / nSampleRate),
	m_Audio1 (GPIOPinAudioLeft, GPIOModeAlternateFunction0),
	m_Audio2 (GPIOPinAudioRight, GPIOModeAlternateFunction0),
	m_Clock (GPIOClockPWM, GPIOClockSourcePLLD),
	m_bIRQConnected (FALSE),
	m_State (PWMSoundIdle)
{
	assert (m_pInterruptSystem != 0);
	assert (m_nChunkSize > 0);
	assert ((m_nChunkSize & 1) == 0);

	// setup and concatenate DMA buffers and control blocks
	SetupDMAControlBlock (0);
	SetupDMAControlBlock (1);
	m_pControlBlock[0]->nNextControlBlockAddress = (u32) m_pControlBlock[1] + GPU_MEM_BASE;
	m_pControlBlock[1]->nNextControlBlockAddress = (u32) m_pControlBlock[0] + GPU_MEM_BASE;

	// start clock and PWM device
	RunPWM ();

	// enable and reset DMA channel
	PeripheralEntry ();

	assert (!(read32 (ARM_DMACHAN_DEBUG (DMA_CHANNEL_PWM)) & DEBUG_LITE));

	write32 (ARM_DMA_ENABLE, read32 (ARM_DMA_ENABLE) | (1 << DMA_CHANNEL_PWM));
	CTimer::Get ()->usDelay (1000);

	write32 (ARM_DMACHAN_CS (DMA_CHANNEL_PWM), CS_RESET);
	while (read32 (ARM_DMACHAN_CS (DMA_CHANNEL_PWM)) & CS_RESET)
	{
		// do nothing
	}

	PeripheralExit ();
}

CPWMSoundBaseDevice::~CPWMSoundBaseDevice (void)
{
	assert (m_State == PWMSoundIdle);

	// stop PWM device and clock
	StopPWM ();

	// disconnect IRQ
	assert (m_pInterruptSystem != 0);
	if (m_bIRQConnected)
	{
		m_pInterruptSystem->DisconnectIRQ (ARM_IRQ_DMA0+DMA_CHANNEL_PWM);
	}

	m_pInterruptSystem = 0;

	// free buffers
	m_pControlBlock[0] = 0;
	m_pControlBlock[1] = 0;
	delete [] m_pControlBlockBuffer[0];
	m_pControlBlockBuffer[0] = 0;
	delete [] m_pControlBlockBuffer[1];
	m_pControlBlockBuffer[1] = 0;

	delete [] m_pDMABuffer[0];
	m_pDMABuffer[0] = 0;
	delete [] m_pDMABuffer[1];
	m_pDMABuffer[1] = 0;
}

unsigned CPWMSoundBaseDevice::GetRange (void) const
{
	return m_nRange;
}

void CPWMSoundBaseDevice::Start (void)
{
	assert (m_State == PWMSoundIdle);

	// fill buffer 0
	m_nNextBuffer = 0;

	if (!GetNextChunk ())
	{
		return;
	}

	m_State = PWMSoundRunning;

	// connect IRQ
	if (!m_bIRQConnected)
	{
		assert (m_pInterruptSystem != 0);
		m_pInterruptSystem->ConnectIRQ (ARM_IRQ_DMA0+DMA_CHANNEL_PWM, InterruptStub, this);

		m_bIRQConnected = TRUE;
	}

	// enable PWM DMA operation
	PeripheralEntry ();

	write32 (ARM_PWM_DMAC,   ARM_PWM_DMAC_ENAB
			       | (7 << ARM_PWM_DMAC_PANIC__SHIFT)
			       | (7 << ARM_PWM_DMAC_DREQ__SHIFT));

	// switched this on when playback stops to avoid clicks, switch it off here
	write32 (ARM_PWM_CTL, read32 (ARM_PWM_CTL) & ~(ARM_PWM_CTL_RPTL1 | ARM_PWM_CTL_RPTL2));

	PeripheralExit ();

	// start DMA
	PeripheralEntry ();

	assert (!(read32 (ARM_DMACHAN_CS (DMA_CHANNEL_PWM)) & CS_INT));
	assert (!(read32 (ARM_DMA_INT_STATUS) & (1 << DMA_CHANNEL_PWM)));

	assert (m_pControlBlock[0] != 0);
	write32 (ARM_DMACHAN_CONBLK_AD (DMA_CHANNEL_PWM), (u32) m_pControlBlock[0] + GPU_MEM_BASE);


	write32 (ARM_DMACHAN_CS (DMA_CHANNEL_PWM),   CS_WAIT_FOR_OUTSTANDING_WRITES
					           | (DEFAULT_PANIC_PRIORITY << CS_PANIC_PRIORITY_SHIFT)
					           | (DEFAULT_PRIORITY << CS_PRIORITY_SHIFT)
					           | CS_ACTIVE);

	PeripheralExit ();

	// fill buffer 1
	if (!GetNextChunk ())
	{
		m_SpinLock.Acquire ();

		if (m_State == PWMSoundRunning)
		{
			PeripheralEntry ();
			write32 (ARM_DMACHAN_NEXTCONBK (DMA_CHANNEL_PWM), 0);
			PeripheralExit ();

			m_State = PWMSoundTerminating;
		}

		m_SpinLock.Release ();
	}
}

void CPWMSoundBaseDevice::Cancel (void)
{
	m_SpinLock.Acquire ();

	if (m_State == PWMSoundRunning)
	{
		m_State = PWMSoundCancelled;
	}

	m_SpinLock.Release ();
}

boolean CPWMSoundBaseDevice::IsActive (void) const
{
	return m_State != PWMSoundIdle ? TRUE : FALSE;
}

boolean CPWMSoundBaseDevice::GetNextChunk (void)
{
	assert (m_pDMABuffer[m_nNextBuffer] != 0);
	unsigned nChunkSize = GetChunk (m_pDMABuffer[m_nNextBuffer], m_nChunkSize);
	if (nChunkSize == 0)
	{
		return FALSE;
	}

	unsigned nTransferLength = nChunkSize * sizeof (u32);
	assert (nTransferLength <= TXFR_LEN_MAX);

	assert (m_pControlBlock[m_nNextBuffer] != 0);
	m_pControlBlock[m_nNextBuffer]->nTransferLength = nTransferLength;

	CleanAndInvalidateDataCacheRange ((u32) m_pDMABuffer[m_nNextBuffer], nTransferLength);
	CleanAndInvalidateDataCacheRange ((u32) m_pControlBlock[m_nNextBuffer], sizeof (TDMAControlBlock));

	m_nNextBuffer ^= 1;

	return TRUE;
}

void CPWMSoundBaseDevice::RunPWM (void)
{
	PeripheralEntry ();

	m_Clock.Start (CLOCK_DIVIDER);
	CTimer::SimpleusDelay (2000);

	assert ((1 << 8) <= m_nRange && m_nRange < (1 << 16));
	write32 (ARM_PWM_RNG1, m_nRange);
	write32 (ARM_PWM_RNG2, m_nRange);

	write32 (ARM_PWM_CTL,   ARM_PWM_CTL_PWEN1 | ARM_PWM_CTL_USEF1
			      | ARM_PWM_CTL_PWEN2 | ARM_PWM_CTL_USEF2
			      | ARM_PWM_CTL_CLRF1);
	CTimer::SimpleusDelay (2000);

	PeripheralExit ();
}

void CPWMSoundBaseDevice::StopPWM (void)
{
	PeripheralEntry ();

	write32 (ARM_PWM_DMAC, 0);
	write32 (ARM_PWM_CTL, 0);			// disable PWM channel 0 and 1
	CTimer::SimpleusDelay (2000);

	m_Clock.Stop ();
	CTimer::SimpleusDelay (2000);

	PeripheralExit ();
}

void CPWMSoundBaseDevice::InterruptHandler (void)
{
	assert (m_State != PWMSoundIdle);

	PeripheralEntry ();

#ifndef NDEBUG
	u32 nIntStatus = read32 (ARM_DMA_INT_STATUS);
#endif
	u32 nIntMask = 1 << DMA_CHANNEL_PWM;
	assert (nIntStatus & nIntMask);
	write32 (ARM_DMA_INT_STATUS, nIntMask);

	u32 nCS = read32 (ARM_DMACHAN_CS (DMA_CHANNEL_PWM));
	assert (nCS & CS_INT);
	write32 (ARM_DMACHAN_CS (DMA_CHANNEL_PWM), nCS);	// reset CS_INT

	PeripheralExit ();

	if (nCS & CS_ERROR)
	{
		m_State = PWMSoundError;

		return;
	}

	m_SpinLock.Acquire ();

	switch (m_State)
	{
	case PWMSoundRunning:
		if (GetNextChunk ())
		{
			break;
		}
		// fall through

	case PWMSoundCancelled:
		PeripheralEntry ();
		write32 (ARM_DMACHAN_NEXTCONBK (DMA_CHANNEL_PWM), 0);
		PeripheralExit ();

		// avoid clicks
		PeripheralEntry ();
		write32 (ARM_PWM_CTL, read32 (ARM_PWM_CTL) | ARM_PWM_CTL_RPTL1 | ARM_PWM_CTL_RPTL2);
		PeripheralExit ();

		m_State = PWMSoundTerminating;
		break;

	case PWMSoundTerminating:
		m_State = PWMSoundIdle;
		break;

	default:
		assert (0);
		break;
	}

	m_SpinLock.Release ();
}

void CPWMSoundBaseDevice::InterruptStub (void *pParam)
{
	CPWMSoundBaseDevice *pThis = (CPWMSoundBaseDevice *) pParam;
	assert (pThis != 0);

	pThis->InterruptHandler ();
}

void CPWMSoundBaseDevice::SetupDMAControlBlock (unsigned nID)
{
	assert (nID <= 1);

	m_pDMABuffer[nID] = new u32[m_nChunkSize];
	assert (m_pDMABuffer[nID] != 0);

	m_pControlBlockBuffer[nID] = new u8[sizeof (TDMAControlBlock) + 31];
	assert (m_pControlBlockBuffer[nID] != 0);
	m_pControlBlock[nID] = (TDMAControlBlock *) (((u32) m_pControlBlockBuffer[nID] + 31) & ~31);

	m_pControlBlock[nID]->nTransferInformation     =   (DREQSourcePWM << TI_PERMAP_SHIFT)
						         | (DEFAULT_BURST_LENGTH << TI_BURST_LENGTH_SHIFT)
						         | TI_SRC_WIDTH
						         | TI_SRC_INC
						         | TI_DEST_DREQ
						         | TI_WAIT_RESP
						         | TI_INTEN;
	m_pControlBlock[nID]->nSourceAddress           = (u32) m_pDMABuffer[nID] + GPU_MEM_BASE;
	m_pControlBlock[nID]->nDestinationAddress      = (ARM_PWM_FIF1 & 0xFFFFFF) + GPU_IO_BASE;
	m_pControlBlock[nID]->n2DModeStride            = 0;
	m_pControlBlock[nID]->nReserved[0]	       = 0;
	m_pControlBlock[nID]->nReserved[1]	       = 0;
}
