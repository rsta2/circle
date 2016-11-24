//
// pwmsounddevice.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
//
// Information to implement this is from:
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
#include <circle/pwmsounddevice.h>
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/timer.h>
#include <circle/synchronize.h>
#include <circle/util.h>
#include <assert.h>

// Playback works with 44100 Hz, Stereo, 12 Bit only.
// Other sound formats will be converted on the fly.

#define CLOCK_FREQ		500000000		// PLLD
#define CLOCK_DIVIDER		2
#define SAMPLE_RATE		44100

#define PWM_RANGE		(CLOCK_FREQ / CLOCK_DIVIDER / SAMPLE_RATE)

#define DMA_BUF_SIZE		2048

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

CPWMSoundDevice::CPWMSoundDevice (CInterruptSystem *pInterrupt)
:	m_Audio1 (GPIOPinAudioLeft, GPIOModeAlternateFunction0),
	m_Audio2 (GPIOPinAudioRight, GPIOModeAlternateFunction0),
	m_Clock (GPIOClockPWM, GPIOClockSourcePLLD),
	m_DMAChannel (DMA_CHANNEL_PWM, pInterrupt),
	m_pSoundData (0),
	m_bActive (FALSE)
{
	m_pDMABuffer = new u32[DMA_BUF_SIZE];
	assert (m_pDMABuffer != 0);

	PeripheralEntry ();
	Run ();
	PeripheralExit ();
}

CPWMSoundDevice::~CPWMSoundDevice (void)
{
	assert (!m_bActive);

	PeripheralEntry ();
	Stop ();
	PeripheralExit ();

	delete m_pDMABuffer;
	m_pDMABuffer = 0;
}

void CPWMSoundDevice::Playback (void *pSoundData, unsigned nSamples, unsigned nChannels, unsigned nBitsPerSample)
{
	assert (!m_bActive);

	assert (pSoundData != 0);
	assert (nChannels == 1 || nChannels == 2);
	assert (nBitsPerSample == 8 || nBitsPerSample == 16);

	m_pSoundData	 = (u8 *) pSoundData;
	m_nSamples	 = nSamples;
	m_nChannels	 = nChannels;
	m_nBitsPerSample = nBitsPerSample;

	unsigned nTransferSize = ConvertSample ();
	if (nTransferSize == 0)
	{
		return;
	}

	m_bActive = TRUE;

	assert (m_pDMABuffer != 0);
	m_DMAChannel.SetupIOWrite (ARM_PWM_FIF1 - ARM_IO_BASE + GPU_IO_BASE,
				   m_pDMABuffer, nTransferSize, DREQSourcePWM);
	m_DMAChannel.SetCompletionRoutine (DMACompletionStub, this);

	PeripheralEntry ();

	write32 (ARM_PWM_DMAC,   ARM_PWM_DMAC_ENAB
			       | (7 << ARM_PWM_DMAC_PANIC__SHIFT)
			       | (7 << ARM_PWM_DMAC_DREQ__SHIFT));

	m_DMAChannel.Start ();

	// switched this on when playback stops to avoid clicks, switch it off here
	write32 (ARM_PWM_CTL, read32 (ARM_PWM_CTL) & ~(ARM_PWM_CTL_RPTL1 | ARM_PWM_CTL_RPTL2));

	PeripheralExit ();
}

boolean CPWMSoundDevice::PlaybackActive (void) const
{
	return m_bActive;
}

void CPWMSoundDevice::CancelPlayback (void)
{
	m_nSamples = 0;
}

void CPWMSoundDevice::Run (void)
{
	m_Clock.Start (CLOCK_DIVIDER);
	CTimer::SimpleusDelay (2000);

	write32 (ARM_PWM_RNG1, PWM_RANGE);
	write32 (ARM_PWM_RNG2, PWM_RANGE);
	
	write32 (ARM_PWM_CTL,   ARM_PWM_CTL_PWEN1 | ARM_PWM_CTL_USEF1
			      | ARM_PWM_CTL_PWEN2 | ARM_PWM_CTL_USEF2
			      | ARM_PWM_CTL_CLRF1);
	CTimer::SimpleusDelay (2000);
}

void CPWMSoundDevice::Stop (void)
{
	write32 (ARM_PWM_DMAC, 0);
	write32 (ARM_PWM_CTL, 0);			// disable PWM channel 0 and 1
	CTimer::SimpleusDelay (2000);

	m_Clock.Stop ();
	CTimer::SimpleusDelay (2000);
}

unsigned CPWMSoundDevice::ConvertSample (void)
{
	unsigned nResult = 0;

	if (m_nSamples == 0)
	{
		return nResult;
	}

	assert (m_pSoundData != 0);
	assert (m_nChannels == 1 || m_nChannels == 2);
	assert (m_nBitsPerSample == 8 || m_nBitsPerSample == 16);

	for (unsigned nSample = 0; nSample < DMA_BUF_SIZE / 2;)		// 2 channels on output
	{
		unsigned nValue = *m_pSoundData++;
		if (m_nBitsPerSample > 8)
		{
			nValue |= (unsigned) *m_pSoundData++ << 8;
			nValue = (nValue + 0x8000) & 0xFFFF;		// signed -> unsigned (16 bit)
		}
		
		if (m_nBitsPerSample >= 12)
		{
			nValue >>= m_nBitsPerSample - 12;
		}
		else
		{
			nValue <<= 12 - m_nBitsPerSample;
		}

		m_pDMABuffer[nSample++] = nValue;

		if (m_nChannels == 2)
		{
			nValue = *m_pSoundData++;
			if (m_nBitsPerSample > 8)
			{
				nValue |= (unsigned) *m_pSoundData++ << 8;
				nValue = (nValue + 0x8000) & 0xFFFF;	// signed -> unsigned (16 bit)
			}

			if (m_nBitsPerSample >= 12)
			{
				nValue >>= m_nBitsPerSample - 12;
			}
			else
			{
				nValue <<= 12 - m_nBitsPerSample;
			}
		}

		m_pDMABuffer[nSample++] = nValue;

		nResult += sizeof (u32) * 2;

		if (--m_nSamples == 0)
		{
			break;
		}
	}

	return nResult;
}

void CPWMSoundDevice::DMACompletionRoutine (boolean bStatus)
{
	assert (m_bActive);

	unsigned nTransferSize;
	if (   !bStatus
	    || (nTransferSize = ConvertSample ()) == 0)
	{
		// avoid clicks
		write32 (ARM_PWM_CTL, read32 (ARM_PWM_CTL) | ARM_PWM_CTL_RPTL1 | ARM_PWM_CTL_RPTL2);

		m_bActive = FALSE;

		return;
	}

	assert (m_pDMABuffer != 0);
	m_DMAChannel.SetupIOWrite (ARM_PWM_FIF1 - ARM_IO_BASE + GPU_IO_BASE,
				   m_pDMABuffer, nTransferSize, DREQSourcePWM);
	m_DMAChannel.SetCompletionRoutine (DMACompletionStub, this);

	m_DMAChannel.Start ();
}

void CPWMSoundDevice::DMACompletionStub (unsigned nChannel, boolean bStatus, void *pParam)
{
	CPWMSoundDevice *pThis = (CPWMSoundDevice *) pParam;
	assert (pThis != 0);

	pThis->DMACompletionRoutine (bStatus);
}
