//
// pwmoutput.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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
#include <circle/bcm2835.h>
#include <circle/timer.h>
#include <circle/memio.h>
#include <circle/synchronize.h>
#include <assert.h>

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

CPWMOutput::CPWMOutput (TGPIOClockSource Source, unsigned nDivider, unsigned nRange, boolean bMSMode)
:	m_Clock (GPIOClockPWM, Source),
	m_nDivider (nDivider),
	m_nRange (nRange),
	m_bMSMode (bMSMode),
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

void CPWMOutput::Start (void)
{
	assert (!m_bActive);

	assert (1 <= m_nDivider && m_nDivider <= 4095);
	m_Clock.Start (m_nDivider);
	CTimer::SimpleusDelay (2000);

	PeripheralEntry ();

	write32 (ARM_PWM_RNG1, m_nRange);
	write32 (ARM_PWM_RNG2, m_nRange);
	
	u32 nControl = ARM_PWM_CTL_PWEN1 | ARM_PWM_CTL_PWEN2;
	if (m_bMSMode)
	{
		nControl |= ARM_PWM_CTL_MSEN1 | ARM_PWM_CTL_MSEN2;
	}

	write32 (ARM_PWM_CTL, nControl);
	CTimer::SimpleusDelay (2000);

	PeripheralExit ();

	m_bActive = TRUE;
}

void CPWMOutput::Stop (void)
{
	assert (m_bActive);
	m_bActive = FALSE;

	m_Clock.Stop ();
	CTimer::SimpleusDelay (2000);

	PeripheralEntry ();

	write32 (ARM_PWM_CTL, 0);
	CTimer::SimpleusDelay (2000);

	PeripheralExit ();
}

void CPWMOutput::Write (unsigned nChannel, unsigned nValue)
{
	assert (m_bActive);
	assert (   nChannel == PWM_CHANNEL1
		|| nChannel == PWM_CHANNEL2);
	assert (nValue <= m_nRange);

	m_SpinLock.Acquire ();

	PeripheralEntry ();

	if (read32 (ARM_PWM_STA) & ARM_PWM_STA_BERR)
	{
		write32 (ARM_PWM_STA, ARM_PWM_STA_BERR);
	}

	write32 (nChannel == PWM_CHANNEL1 ? ARM_PWM_DAT1 : ARM_PWM_DAT2, nValue);
	
	PeripheralExit ();

	m_SpinLock.Release ();
}
