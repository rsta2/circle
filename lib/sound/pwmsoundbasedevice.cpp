//
// pwmsoundbasedevice.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2022  R. Stange <rsta2@o2online.de>
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
#include <circle/sound/pwmsoundbasedevice.h>
#include <circle/devicenameservice.h>
#include <circle/bcm2835.h>
#include <circle/bcm2835int.h>
#include <circle/memio.h>
#include <circle/timer.h>
#include <circle/synchronize.h>
#include <circle/machineinfo.h>
#include <circle/util.h>
#include <circle/new.h>
#include <assert.h>

//
// PWM device selection
//
#if RASPPI <= 3
	#define CLOCK_RATE	250000000
	#define PWM_BASE	ARM_PWM_BASE
	#define DREQ_SOURCE	DREQSourcePWM
#else
	#define CLOCK_RATE	125000000
	#define PWM_BASE	ARM_PWM1_BASE
	#define DREQ_SOURCE	DREQSourcePWM1
#endif

//
// PWM register offsets
//
#define PWM_CTL			(PWM_BASE + 0x00)
#define PWM_STA			(PWM_BASE + 0x04)
#define PWM_DMAC		(PWM_BASE + 0x08)
#define PWM_RNG1		(PWM_BASE + 0x10)
#define PWM_DAT1		(PWM_BASE + 0x14)
#define PWM_FIF1		(PWM_BASE + 0x18)
#define PWM_RNG2		(PWM_BASE + 0x20)
#define PWM_DAT2		(PWM_BASE + 0x24)

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
#define ARM_PWM_CTL_MSEN2	(1 << 15)

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

CPWMSoundBaseDevice::CPWMSoundBaseDevice (CInterruptSystem *pInterrupt,
					  unsigned	    nSampleRate,
					  unsigned	    nChunkSize)
:	CSoundBaseDevice (SoundFormatUnsigned32,
			  (CLOCK_RATE + nSampleRate/2) / nSampleRate, nSampleRate,
			  CMachineInfo::Get ()->ArePWMChannelsSwapped ()),
	m_pInterruptSystem (pInterrupt),
	m_nChunkSize (nChunkSize),
	m_nRange ((CLOCK_RATE + nSampleRate/2) / nSampleRate),
#ifdef USE_GPIO18_FOR_LEFT_PWM_ON_ZERO
	m_Audio1 (GPIOPinAudioLeft, GPIOModeAlternateFunction5),
#else
	m_Audio1 (GPIOPinAudioLeft, GPIOModeAlternateFunction0),
#endif // USE_GPIO18_FOR_LEFT_PWM_ON_ZERO
#ifdef USE_GPIO19_FOR_RIGHT_PWM_ON_ZERO
	m_Audio2 (GPIOPinAudioRight, GPIOModeAlternateFunction5),
#else
	m_Audio2 (GPIOPinAudioRight, GPIOModeAlternateFunction0),
#endif // USE_GPIO19_FOR_RIGHT_PWM_ON_ZERO
	m_Clock (GPIOClockPWM),
	m_bIRQConnected (FALSE),
	m_State (PWMSoundIdle),
	m_nDMAChannel (CMachineInfo::Get ()->AllocateDMAChannel (DMA_CHANNEL_LITE))
{
	assert (m_pInterruptSystem != 0);
	assert (m_nChunkSize > 0);
	assert ((m_nChunkSize & 1) == 0);

	// setup and concatenate DMA buffers and control blocks
	SetupDMAControlBlock (0);
	SetupDMAControlBlock (1);
	m_pControlBlock[0]->nNextControlBlockAddress = BUS_ADDRESS ((uintptr) m_pControlBlock[1]);
	m_pControlBlock[1]->nNextControlBlockAddress = BUS_ADDRESS ((uintptr) m_pControlBlock[0]);

	// start clock and PWM device
	RunPWM ();

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

	CDeviceNameService::Get ()->AddDevice ("sndpwm", this, FALSE);
}

CPWMSoundBaseDevice::~CPWMSoundBaseDevice (void)
{
	assert (m_State == PWMSoundIdle);

	CDeviceNameService::Get ()->RemoveDevice ("sndpwm", FALSE);

	// stop PWM device and clock
	StopPWM ();

	// reset and disable DMA channel
	PeripheralEntry ();

	assert (m_nDMAChannel <= DMA_CHANNEL_MAX);

	write32 (ARM_DMACHAN_CS (m_nDMAChannel), CS_RESET);
	while (read32 (ARM_DMACHAN_CS (m_nDMAChannel)) & CS_RESET)
	{
		// do nothing
	}

	write32 (ARM_DMA_ENABLE, read32 (ARM_DMA_ENABLE) & ~(1 << m_nDMAChannel));

	PeripheralExit ();

	// disconnect IRQ
	assert (m_pInterruptSystem != 0);
	if (m_bIRQConnected)
	{
		m_pInterruptSystem->DisconnectIRQ (ARM_IRQ_DMA0+m_nDMAChannel);
	}

	m_pInterruptSystem = 0;

	// free DMA channel
	CMachineInfo::Get ()->FreeDMAChannel (m_nDMAChannel);

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

int CPWMSoundBaseDevice::GetRangeMin (void) const
{
	return 0;
}

int CPWMSoundBaseDevice::GetRangeMax (void) const
{
	return (int) (m_nRange-1);
}

boolean CPWMSoundBaseDevice::Start (void)
{
	assert (m_State == PWMSoundIdle);

	// fill buffer 0
	m_nNextBuffer = 0;

	if (!GetNextChunk ())
	{
		return FALSE;
	}

	m_State = PWMSoundRunning;

	// connect IRQ
	assert (m_nDMAChannel <= DMA_CHANNEL_MAX);

	if (!m_bIRQConnected)
	{
		assert (m_pInterruptSystem != 0);
		m_pInterruptSystem->ConnectIRQ (ARM_IRQ_DMA0+m_nDMAChannel, InterruptStub, this);

		m_bIRQConnected = TRUE;
	}

	// enable PWM DMA operation
	PeripheralEntry ();

	write32 (PWM_DMAC,   ARM_PWM_DMAC_ENAB
			   | (7 << ARM_PWM_DMAC_PANIC__SHIFT)
			   | (7 << ARM_PWM_DMAC_DREQ__SHIFT));

	// switched this on when playback stops to avoid clicks, switch it off here
	write32 (PWM_CTL, read32 (PWM_CTL) & ~(ARM_PWM_CTL_RPTL1 | ARM_PWM_CTL_RPTL2));

	PeripheralExit ();

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
	if (!GetNextChunk ())
	{
		m_SpinLock.Acquire ();

		if (m_State == PWMSoundRunning)
		{
			PeripheralEntry ();
			write32 (ARM_DMACHAN_NEXTCONBK (m_nDMAChannel), 0);
			PeripheralExit ();

			m_State = PWMSoundTerminating;
		}

		m_SpinLock.Release ();
	}

	return TRUE;
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
	assert (nTransferLength <= TXFR_LEN_MAX_LITE);

	assert (m_pControlBlock[m_nNextBuffer] != 0);
	m_pControlBlock[m_nNextBuffer]->nTransferLength = nTransferLength;

	CleanAndInvalidateDataCacheRange ((uintptr) m_pDMABuffer[m_nNextBuffer], nTransferLength);
	CleanAndInvalidateDataCacheRange ((uintptr) m_pControlBlock[m_nNextBuffer], sizeof (TDMAControlBlock));

	m_nNextBuffer ^= 1;

	return TRUE;
}

void CPWMSoundBaseDevice::RunPWM (void)
{
	PeripheralEntry ();

#ifndef NDEBUG
	boolean bOK =
#endif
		m_Clock.StartRate (CLOCK_RATE);
	assert (bOK);
	CTimer::SimpleusDelay (2000);

	assert ((1 << 8) <= m_nRange && m_nRange < (1 << 16));
	write32 (PWM_RNG1, m_nRange);
	write32 (PWM_RNG2, m_nRange);

	write32 (PWM_CTL,   ARM_PWM_CTL_PWEN1 | ARM_PWM_CTL_USEF1
			  | ARM_PWM_CTL_PWEN2 | ARM_PWM_CTL_USEF2
			  | ARM_PWM_CTL_CLRF1);
	CTimer::SimpleusDelay (2000);

	PeripheralExit ();
}

void CPWMSoundBaseDevice::StopPWM (void)
{
	PeripheralEntry ();

	write32 (PWM_DMAC, 0);
	write32 (PWM_CTL, 0);			// disable PWM channel 0 and 1
	CTimer::SimpleusDelay (2000);

	m_Clock.Stop ();
	CTimer::SimpleusDelay (2000);

	PeripheralExit ();
}

void CPWMSoundBaseDevice::InterruptHandler (void)
{
	assert (m_State != PWMSoundIdle);
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
		write32 (ARM_DMACHAN_NEXTCONBK (m_nDMAChannel), 0);
		PeripheralExit ();

		// avoid clicks
		PeripheralEntry ();
		write32 (PWM_CTL, read32 (PWM_CTL) | ARM_PWM_CTL_RPTL1 | ARM_PWM_CTL_RPTL2);
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

	m_pDMABuffer[nID] = new (HEAP_DMA30) u32[m_nChunkSize];
	assert (m_pDMABuffer[nID] != 0);

	m_pControlBlockBuffer[nID] = new (HEAP_DMA30) u8[sizeof (TDMAControlBlock) + 31];
	assert (m_pControlBlockBuffer[nID] != 0);
	m_pControlBlock[nID] = (TDMAControlBlock *) (((uintptr) m_pControlBlockBuffer[nID] + 31) & ~31);

	m_pControlBlock[nID]->nTransferInformation     =   (DREQ_SOURCE << TI_PERMAP_SHIFT)
						         | (DEFAULT_BURST_LENGTH << TI_BURST_LENGTH_SHIFT)
						         | TI_SRC_WIDTH
						         | TI_SRC_INC
						         | TI_DEST_DREQ
						         | TI_WAIT_RESP
						         | TI_INTEN;
	m_pControlBlock[nID]->nSourceAddress           = BUS_ADDRESS ((uintptr) m_pDMABuffer[nID]);
	m_pControlBlock[nID]->nDestinationAddress      = (PWM_FIF1 & 0xFFFFFF) + GPU_IO_BASE;
	m_pControlBlock[nID]->n2DModeStride            = 0;
	m_pControlBlock[nID]->nReserved[0]	       = 0;
	m_pControlBlock[nID]->nReserved[1]	       = 0;
}
