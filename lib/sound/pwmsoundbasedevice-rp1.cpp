//
// pwmsoundbasedevice-rp1.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2024  R. Stange <rsta2@o2online.de>
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
#include <circle/machineinfo.h>
#include <circle/sysconfig.h>
#include <circle/bcm2712.h>
#include <circle/memio.h>
#include <circle/timer.h>
#include <circle/macros.h>
#include <assert.h>

//
// PWM device selection
//
#define CLOCK_RATE	50000000
#define PWM_BASE	ARM_PWM0_BASE
#define DREQ_SOURCE	CDMAChannelRP1::DREQSourcePWM0

#define DMA_CHANNEL	1			// TODO: allocate dynamically

//
// PWM register offsets
//
#define PWM_GLOBAL_CTRL		(PWM_BASE + 0x00)
	#define PWM_GLOBAL_CTRL_CHAN_EN(chan)	BIT (chan)
	#define PWM_GLOBAL_CTRL_SET_UPDATE	BIT (31)
#define PWM_FIFO_CTRL		(PWM_BASE + 0x04)
	#define PWM_FIFO_CTRL_LEVEL__SHIFT	0
	#define PWM_FIFO_CTRL_LEVEL__MASK	(0x1F << 0)
	#define PWM_FIFO_CTRL_FLUSH		BIT (5)
	#define PWM_FIFO_CTRL_FLUSH_DONE	BIT (6)
	#define PWM_FIFO_CTRL_THRESHOLD__SHIFT	11
	#define PWM_FIFO_CTRL_THRESHOLD__MASK	(0x1F << 11)
		#define PWM_FIFO_CTRL_THRESHOLD__DEFAULT	0x08
	#define PWM_FIFO_CTRL_DWELL_TIME__SHIFT	16
	#define PWM_FIFO_CTRL_DWELL_TIME__MASK	(0x1F << 16)
		#define PWM_FIFO_CTRL_DWELL_TIME__DEFAULT	0x02
	#define PWM_FIFO_CTRL_DREQ_EN		BIT (31)
#define PWM_COMMON_RANGE	(PWM_BASE + 0x08)
#define PWM_COMMON_DUTY		(PWM_BASE + 0x0C)
#define PWM_DUTY_FIFO		(PWM_BASE + 0x10)

#define PWM_CHAN_CTRL(chan)	(PWM_BASE + (chan) * 0x10 + 0x14)
	#define PWM_CHAN_CTRL_MODE__SHIFT	0
	#define PWM_CHAN_CTRL_MODE__MASK	(7 << 0)
		#define PWM_CHAN_CTRL_MODE_ZERO			0
		#define PWM_CHAN_CTRL_MODE_TRAILING_EDGE	1
		#define PWM_CHAN_CTRL_MODE_DOUBLE_EDGE		2
		#define PWM_CHAN_CTRL_MODE_PDM			3
		#define PWM_CHAN_CTRL_MODE_SERIALISER_MSB	4
		#define PWM_CHAN_CTRL_MODE_PPM			5
		#define PWM_CHAN_CTRL_MODE_LEADING_EDGE		6
		#define PWM_CHAN_CTRL_MODE_SERIALISER_LSB	7
	#define PWM_CHAN_CTRL_INVERT		BIT (3)
	#define PWM_CHAN_CTRL_BIND		BIT (4)
	#define PWM_CHAN_CTRL_USEFIFO		BIT (5)
	#define PWM_CHAN_CTRL_SDM		BIT (6)
	#define PWM_CHAN_CTRL_DITHER		BIT (7)
	#define PWM_CHAN_CTRL_FIFO_POP_MASK	BIT (8)
	#define PWM_CHAN_CTRL_SDM_BITWIDTH__SHIFT 12
	#define PWM_CHAN_CTRL_SDM_BITWIDTH__MASK  (0x0F << 12)
	#define PWM_CHAN_CTRL_BIAS__SHIFT	16
	#define PWM_CHAN_CTRL_BIAS__MASK	(0xFFFFU << 16)
#define PWM_CHAN_RANGE(chan)	(PWM_BASE + (chan) * 0x10 + 0x18)
#define PWM_CHAN_PHASE(chan)	(PWM_BASE + (chan) * 0x10 + 0x1C)
#define PWM_CHAN_DUTY(chan)	(PWM_BASE + (chan) * 0x10 + 0x20)

CPWMSoundBaseDevice::CPWMSoundBaseDevice (CInterruptSystem *pInterrupt,
					  unsigned	    nSampleRate,
					  unsigned	    nChunkSize)
:	CSoundBaseDevice (SoundFormatUnsigned32,
			  (CLOCK_RATE + nSampleRate/2) / nSampleRate, nSampleRate,
			  CMachineInfo::Get ()->ArePWMChannelsSwapped ()),
	m_nChunkSize (nChunkSize),
	m_nRange ((CLOCK_RATE + nSampleRate/2) / nSampleRate),
#ifndef USE_GPIO18_FOR_LEFT_PWM
	m_nChannel1 (0),
	m_Audio1 (12, GPIOModeAlternateFunction0),
#else
	m_nChannel1 (2),
	m_Audio1 (18, GPIOModeAlternateFunction3),
#endif
#ifndef USE_GPIO19_FOR_RIGHT_PWM
	m_nChannel2 (1),
	m_Audio2 (13, GPIOModeAlternateFunction0),
#else
	m_nChannel2 (3),
	m_Audio2 (19, GPIOModeAlternateFunction3),
#endif
	m_Clock (GPIOClockPWM0, GPIOClockSourceXOscillator),
	m_State (StateIdle),
	m_DMAChannel (DMA_CHANNEL, pInterrupt),
	m_pDMABuffer {new u32[m_nChunkSize], new u32[m_nChunkSize]}
{
	CDeviceNameService::Get ()->AddDevice ("sndpwm", this, FALSE);
}

CPWMSoundBaseDevice::~CPWMSoundBaseDevice (void)
{
	assert (   m_State != StateRunning
		&& m_State != StateCanceled);

	CDeviceNameService::Get ()->RemoveDevice ("sndpwm", FALSE);

	StopPWM ();

	delete [] m_pDMABuffer[0];
	delete [] m_pDMABuffer[1];

	m_Audio1.SetMode (GPIOModeNone);
	m_Audio2.SetMode (GPIOModeNone);
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
	if (m_State == StateFailed)
	{
		return FALSE;
	}

	assert (m_State == StateIdle);
	m_State = StateRunning;

	assert (m_pDMABuffer[0]);
	assert (m_pDMABuffer[1]);
	assert (m_nChunkSize);
	assert (!(m_nChunkSize & 1));

	u32 nLevel0 = m_nRange / 2;
	for (unsigned i = 0; i < m_nChunkSize; i++)
	{
		m_pDMABuffer[0][i] = nLevel0;
		m_pDMABuffer[1][i] = nLevel0;
	}

	const void *Buffers[2] = {m_pDMABuffer[0], m_pDMABuffer[1]};
	m_DMAChannel.SetupCyclicIOWrite (PWM_DUTY_FIFO, Buffers, 2,
					 m_nChunkSize * sizeof (u32), DREQ_SOURCE);

	m_DMAChannel.SetCompletionRoutine (DMACompletionStub, this);
	m_DMAChannel.Start ();

	if (!RunPWM ())
	{
		m_DMAChannel.Cancel ();

		m_State = StateFailed;

		return FALSE;
	}

	return TRUE;
}

void CPWMSoundBaseDevice::Cancel (void)
{
	m_SpinLock.Acquire ();

	if (m_State == StateRunning)
	{
		m_State = StateCanceled;

		m_SpinLock.Release ();

		m_DMAChannel.Cancel ();
		StopPWM ();

		m_SpinLock.Acquire ();

		m_State = StateIdle;
	}

	m_SpinLock.Release ();
}

boolean CPWMSoundBaseDevice::IsActive (void) const
{
	TState State = m_State;

	return    State == StateRunning
	       || State == StateCanceled;
}

boolean CPWMSoundBaseDevice::RunPWM (void)
{
	if (!m_Clock.StartRate (CLOCK_RATE))
	{
		return FALSE;
	}

	assert (!(read32 (PWM_GLOBAL_CTRL) & PWM_GLOBAL_CTRL_CHAN_EN (m_nChannel1)));
	write32 (PWM_CHAN_CTRL (m_nChannel1),	   PWM_CHAN_CTRL_MODE_TRAILING_EDGE
						<< PWM_CHAN_CTRL_MODE__SHIFT
					      | PWM_CHAN_CTRL_BIND
				              | PWM_CHAN_CTRL_USEFIFO
				              | PWM_CHAN_CTRL_FIFO_POP_MASK);
	write32 (PWM_CHAN_PHASE (m_nChannel1), 0);

	assert (!(read32 (PWM_GLOBAL_CTRL) & PWM_GLOBAL_CTRL_CHAN_EN (m_nChannel2)));
	write32 (PWM_CHAN_CTRL (m_nChannel2),	   PWM_CHAN_CTRL_MODE_TRAILING_EDGE
						<< PWM_CHAN_CTRL_MODE__SHIFT
					      | PWM_CHAN_CTRL_BIND
					      | PWM_CHAN_CTRL_USEFIFO
					      | PWM_CHAN_CTRL_FIFO_POP_MASK);
	write32 (PWM_CHAN_PHASE (m_nChannel2), 0);

	u32 nValue = read32 (PWM_FIFO_CTRL);
	nValue &= ~(PWM_FIFO_CTRL_THRESHOLD__MASK | PWM_FIFO_CTRL_DWELL_TIME__MASK);
	nValue |= PWM_FIFO_CTRL_THRESHOLD__DEFAULT << PWM_FIFO_CTRL_THRESHOLD__SHIFT;
	nValue |= PWM_FIFO_CTRL_DWELL_TIME__DEFAULT << PWM_FIFO_CTRL_DWELL_TIME__SHIFT;
	nValue |= PWM_FIFO_CTRL_FLUSH;
	write32 (PWM_FIFO_CTRL, nValue);

	unsigned nStartTicks = CTimer::GetClockTicks ();
	while (!(read32 (PWM_FIFO_CTRL) & PWM_FIFO_CTRL_FLUSH_DONE))
	{
		if (CTimer::GetClockTicks () - nStartTicks > CLOCKHZ / 10)
		{
			write32 (PWM_FIFO_CTRL, read32 (PWM_FIFO_CTRL) & ~PWM_FIFO_CTRL_FLUSH);

			return FALSE;
		}
	}
	write32 (PWM_FIFO_CTRL, read32 (PWM_FIFO_CTRL) & ~PWM_FIFO_CTRL_FLUSH);

	write32 (PWM_COMMON_RANGE, m_nRange);

	write32 (PWM_GLOBAL_CTRL, read32 (PWM_GLOBAL_CTRL) | PWM_GLOBAL_CTRL_CHAN_EN (m_nChannel1)
							   | PWM_GLOBAL_CTRL_CHAN_EN (m_nChannel2));
	write32 (PWM_GLOBAL_CTRL, read32 (PWM_GLOBAL_CTRL) | PWM_GLOBAL_CTRL_SET_UPDATE);

	write32 (PWM_FIFO_CTRL, read32 (PWM_FIFO_CTRL) | PWM_FIFO_CTRL_DREQ_EN);

	return TRUE;
}

void CPWMSoundBaseDevice::StopPWM (void)
{
	write32 (PWM_FIFO_CTRL, read32 (PWM_FIFO_CTRL) & ~PWM_FIFO_CTRL_DREQ_EN);

	write32 (PWM_GLOBAL_CTRL,    read32 (PWM_GLOBAL_CTRL)
				  & ~(  PWM_GLOBAL_CTRL_CHAN_EN (m_nChannel1)
				      | PWM_GLOBAL_CTRL_CHAN_EN (m_nChannel2)));
	write32 (PWM_GLOBAL_CTRL, read32 (PWM_GLOBAL_CTRL) | PWM_GLOBAL_CTRL_SET_UPDATE);

	write32 (PWM_CHAN_CTRL (m_nChannel1), 0);
	write32 (PWM_CHAN_CTRL (m_nChannel2), 0);

	write32 (PWM_FIFO_CTRL, read32 (PWM_FIFO_CTRL) | PWM_FIFO_CTRL_FLUSH);
	unsigned nStartTicks = CTimer::GetClockTicks ();
	while (!(read32 (PWM_FIFO_CTRL) & PWM_FIFO_CTRL_FLUSH_DONE))
	{
		if (CTimer::GetClockTicks () - nStartTicks > CLOCKHZ / 10)
		{
			break;
		}
	}
	write32 (PWM_FIFO_CTRL, read32 (PWM_FIFO_CTRL) & ~PWM_FIFO_CTRL_FLUSH);

	m_Clock.Stop ();
}

void CPWMSoundBaseDevice::DMACompletionRoutine (unsigned nChannel, unsigned nBuffer, boolean bStatus)
{
	m_SpinLock.Acquire ();

	if (m_State != StateRunning)
	{
		m_SpinLock.Release ();

		return;
	}

	if (!bStatus)
	{
		m_State = StateFailed;

		m_SpinLock.Release ();

		m_DMAChannel.Cancel ();
		StopPWM ();

		return;
	}

	assert (nBuffer < 2);
	assert (m_pDMABuffer[nBuffer]);
	assert (m_nChunkSize);

	if (GetChunk (m_pDMABuffer[nBuffer], m_nChunkSize) < m_nChunkSize)
	{
		m_DMAChannel.Cancel ();
		StopPWM ();

		m_State = StateIdle;
	}

	m_SpinLock.Release ();
}

void CPWMSoundBaseDevice::DMACompletionStub (unsigned nChannel, unsigned nBuffer,
					     boolean bStatus, void *pParam)
{
	CPWMSoundBaseDevice *pThis = static_cast<CPWMSoundBaseDevice *> (pParam);
	assert (pThis);

	pThis->DMACompletionRoutine (nChannel, nBuffer, bStatus);
}
