//
// pwmoutput-rp1.cpp
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
#include <circle/pwmoutput.h>
#include <circle/bcm2712.h>
#include <circle/timer.h>
#include <circle/memio.h>
#include <circle/macros.h>
#include <assert.h>

#define CHANNELS	4

//
// PWM register offsets
//
#define PWM_GLOBAL_CTRL		(m_ulBase + 0x00)
	#define PWM_GLOBAL_CTRL_CHAN_EN(chan)	BIT (chan)
	#define PWM_GLOBAL_CTRL_SET_UPDATE	BIT (31)
#define PWM_FIFO_CTRL		(m_ulBase + 0x04)
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
#define PWM_COMMON_RANGE	(m_ulBase + 0x08)
#define PWM_COMMON_DUTY		(m_ulBase + 0x0C)
#define PWM_DUTY_FIFO		(m_ulBase + 0x10)

#define PWM_CHAN_CTRL(chan)	(m_ulBase + (chan) * 0x10 + 0x14)
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
#define PWM_CHAN_RANGE(chan)	(m_ulBase + (chan) * 0x10 + 0x18)
#define PWM_CHAN_PHASE(chan)	(m_ulBase + (chan) * 0x10 + 0x1C)
#define PWM_CHAN_DUTY(chan)	(m_ulBase + (chan) * 0x10 + 0x20)

CPWMOutput::CPWMOutput (unsigned nClockRateHz, unsigned nRange, boolean bMSMode,
			boolean bInvert, unsigned nDevice)
:	m_Clock (!nDevice ? GPIOClockPWM0 : GPIOClockPWM1),
	m_nClockRateHz (nClockRateHz),
	m_nDivider (0),
	m_nRange (nRange),
	m_bMSMode (bMSMode),
	m_bInvert (bInvert),
	m_ulBase (!nDevice ? ARM_PWM0_BASE : ARM_PWM1_BASE),
	m_bActive (FALSE)
{
}

CPWMOutput::CPWMOutput (TGPIOClockSource Source, unsigned nDivider, unsigned nRange, boolean bMSMode,
			boolean bInvert, unsigned nDevice)
:	m_Clock (!nDevice ? GPIOClockPWM0 : GPIOClockPWM1, Source),
	m_nClockRateHz (0),
	m_nDivider (nDivider),
	m_nRange (nRange),
	m_bMSMode (bMSMode),
	m_bInvert (bInvert),
	m_ulBase (!nDevice ? ARM_PWM0_BASE : ARM_PWM1_BASE),
	m_bActive (FALSE)
{
}

CPWMOutput::~CPWMOutput (void)
{
	if (m_bActive)
	{
		Stop ();
	}
}

boolean CPWMOutput::Start (void)
{
	assert (!m_bActive);

	if (m_nDivider)
	{
		if (!m_Clock.Start (m_nDivider))
		{
			return FALSE;
		}
	}
	else
	{
		assert (m_nClockRateHz);
		if (!m_Clock.StartRate (m_nClockRateHz))
		{
			return FALSE;
		}
	}

	for (unsigned i = 0; i < CHANNELS; i++)
	{
		assert (!(read32 (PWM_GLOBAL_CTRL) & PWM_GLOBAL_CTRL_CHAN_EN (i)));

		write32 (PWM_CHAN_CTRL (i),	(  m_bMSMode
						 ? PWM_CHAN_CTRL_MODE_TRAILING_EDGE
						 : PWM_CHAN_CTRL_MODE_PDM)
					      << PWM_CHAN_CTRL_MODE__SHIFT
					    | (m_bInvert ? PWM_CHAN_CTRL_INVERT : 0));
		write32 (PWM_CHAN_RANGE (i), m_nRange);
		write32 (PWM_CHAN_PHASE (i), 0);
		write32 (PWM_CHAN_DUTY (i), 0);

		write32 (PWM_GLOBAL_CTRL, read32 (PWM_GLOBAL_CTRL) | PWM_GLOBAL_CTRL_CHAN_EN (i));
	}

	write32 (PWM_GLOBAL_CTRL, read32 (PWM_GLOBAL_CTRL) | PWM_GLOBAL_CTRL_SET_UPDATE);

	m_bActive = TRUE;

	return TRUE;
}

void CPWMOutput::Stop (void)
{
	assert (m_bActive);
	m_bActive = FALSE;

	write32 (PWM_GLOBAL_CTRL, 0);
	write32 (PWM_GLOBAL_CTRL, read32 (PWM_GLOBAL_CTRL) | PWM_GLOBAL_CTRL_SET_UPDATE);

	for (unsigned i = 0; i < CHANNELS; i++)
	{
		write32 (PWM_CHAN_CTRL (i), 0);
	}

	write32 (PWM_GLOBAL_CTRL, read32 (PWM_GLOBAL_CTRL) | PWM_GLOBAL_CTRL_SET_UPDATE);

	m_Clock.Stop ();
}

void CPWMOutput::Write (unsigned nChannel, unsigned nValue)
{
	assert (m_bActive);
	assert (   nChannel >= PWM_CHANNEL1
		|| nChannel <= PWM_CHANNEL4);
	assert (nValue <= m_nRange);

	write32 (PWM_CHAN_DUTY (nChannel - PWM_CHANNEL1), nValue);
}
