//
// pwmsoundbasedevice.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2025  R. Stange <rsta2@gmx.net>
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
#include <circle/memio.h>
#include <circle/timer.h>
#include <circle/synchronize.h>
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
	m_bError (FALSE),
	m_DMABuffers (TRUE, PWM_FIF1, DREQ_SOURCE, nChunkSize, pInterrupt)
{
	assert (m_nChunkSize > 0);
	assert ((m_nChunkSize & 1) == 0);

	// start clock and PWM device
	RunPWM ();

	CDeviceNameService::Get ()->AddDevice ("sndpwm", this, FALSE);
}

CPWMSoundBaseDevice::~CPWMSoundBaseDevice (void)
{
	CDeviceNameService::Get ()->RemoveDevice ("sndpwm", FALSE);

	// stop PWM device and clock
	StopPWM ();
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
	if (m_bError)
	{
		return FALSE;
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
	if (!m_DMABuffers.Start (ChunkCompletedHandler, this))
	{
		m_bError = TRUE;

		return FALSE;
	}

	return TRUE;
}

void CPWMSoundBaseDevice::Cancel (void)
{
	m_DMABuffers.Cancel ();
}

boolean CPWMSoundBaseDevice::IsActive (void) const
{
	return m_DMABuffers.IsActive ();
}

void CPWMSoundBaseDevice::Flush(void)
{
	CSoundBaseDevice::Flush ();

	// Clear PWM FIFO
	PeripheralEntry();
	write32(PWM_CTL, read32(PWM_CTL) | ARM_PWM_CTL_CLRF1);
	CTimer::Get()->usDelay(10);
	PeripheralExit();

	// Zero both DMA buffers directly
	m_DMABuffers.ZeroBuffers ();
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

unsigned CPWMSoundBaseDevice::ChunkCompletedHandler (boolean bStatus, u32 *pBuffer,
						     unsigned nChunkSize, void *pParam)
{
	CPWMSoundBaseDevice *pThis = (CPWMSoundBaseDevice *) pParam;
	assert (pThis != 0);

	if (!bStatus)
	{
		pThis->m_bError = TRUE;

		return 0;
	}

	return pThis->GetChunk (pBuffer, nChunkSize);
}
