//
// gpioclock-rp1.h
//
// Portions taken from the Linux driver:
//	driver/clk/clk-rp1.c
//	Clock driver for RP1 PCIe multifunction chip.
//	Copyright (C) 2023 Raspberry Pi Ltd.
//	SPDX-License-Identifier: GPL-2.0+
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
#include <circle/gpioclock.h>
#include <circle/bcm2712.h>
#include <circle/memio.h>
#include <circle/logger.h>
#include <circle/machineinfo.h>
#include <assert.h>

#define CLK_CTRL(clk)			(ARM_GPIO_CLK_BASE + 0x10 + (clk)*0x10 + 0x04)
	#define CLK_CTRL_ENABLE__MASK	BIT(11)
	#define CLK_CTRL_AUXSRC__SHIFT	5
	#define CLK_CTRL_AUXSRC__MASK	(0x0F << 5)
	#define CLK_CTRL_SRC__SHIFT	0
		#define AUX_SEL			1
#define CLK_DIV_INT(clk)		(ARM_GPIO_CLK_BASE + 0x10 + (clk)*0x10 + 0x08)
	#define DIV_INT_8BIT_MAX	0xFFU
	#define DIV_INT_16BIT_MAX	0xFFFFU
#define CLK_DIV_FRAC(clk)		(ARM_GPIO_CLK_BASE + 0x10 + (clk)*0x10 + 0x0C)
	#define CLK_DIV_FRAC_BITS	16
#define CLK_SEL(clk)			(ARM_GPIO_CLK_BASE + 0x10 + (clk)*0x10 + 0x10)

const u8 CGPIOClock::s_ParentAux[GPIOClockUnknown][MaxParents] =
{
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},

	{0, 0, GPIOClockSourceXOscillator},		// PWM0
	{0, 0, GPIOClockSourceXOscillator},		// PWM1
	{0, 0, 0, 0, GPIOClockSourceXOscillator},	// AudioIn
	{0, 0, 0, GPIOClockSourceXOscillator},		// AudioOut
	{GPIOClockSourceXOscillator, GPIOClockSourcePLLAudio, GPIOClockSourcePLLAudioSec}, // I2S

	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},

	{GPIOClockSourceXOscillator, 0, 0, 0, 0, 0, GPIOClockSourcePLLSys},	// GP0
	{0, 0, 0, 0, 0, 0, GPIOClockSourcePLLSysPriPh},				// GP1
	{0, 0, 0, 0, 0, 0, GPIOClockSourcePLLSysSec, 0, 0, 0, 0, 0, 0, 0, 0, GPIOClockSourceClkSys},	// GP2
};

CGPIOClock::CGPIOClock (TGPIOClock Clock, TGPIOClockSource Source)
:	m_Clock (Clock),
	m_Source (Source),
	m_nDivIntMax (0),
	m_nFreqMax (0),
	m_nAuxSrc (MaxParents)
{
	switch (Clock)
	{
	case GPIOClockPWM0:
	case GPIOClockPWM1:
		m_nDivIntMax = DIV_INT_16BIT_MAX;
		m_nFreqMax = 76800000;
		break;

	case GPIOClockAudioIn:
		m_nDivIntMax = DIV_INT_8BIT_MAX;
		m_nFreqMax = 76800000;
		break;

	case GPIOClockAudioOut:
		m_nDivIntMax = DIV_INT_8BIT_MAX;
		m_nFreqMax = 153600000;
		break;

	case GPIOClockI2S:
		m_nDivIntMax = DIV_INT_8BIT_MAX;
		m_nFreqMax = 50000000;
		break;

	case GPIOClock0:
	case GPIOClock1:
	case GPIOClock2:
		m_nDivIntMax = DIV_INT_16BIT_MAX;
		m_nFreqMax = 100000000;
		break;

	default:
		assert (0);
		break;
	}

	assert (m_Source > GPIOClockSourceNone);
	if (m_Source < GPIOClockSourceUnknown)
	{
		for (unsigned i = 0; i < MaxParents; i++)
		{
			assert (m_Clock < GPIOClockUnknown);
			if (s_ParentAux[m_Clock][i] == m_Source)
			{
				m_nAuxSrc = i;

				return;
			}
		}

		assert (0);	// clock does not support this source
	}
}

CGPIOClock::~CGPIOClock (void)
{
	Stop ();

	m_Clock = GPIOClockUnknown;
}

void CGPIOClock::Start (unsigned nDivI, unsigned nDivF)
{
	assert (m_Clock < GPIOClockUnknown);
	assert (1 <= nDivI && nDivI <= m_nDivIntMax);
	assert (nDivF <= (1 << CLK_DIV_FRAC_BITS)-1);

	Stop ();

	// check maximum frequency
#ifndef NDEBUG
	assert (m_Source < GPIOClockSourceUnknown);
	u64 nSourceRate = CMachineInfo::Get ()->GetGPIOClockSourceRate (m_Source);
	assert (nSourceRate);

	u64 ulDivider = ((u64) nDivI << CLK_DIV_FRAC_BITS) | nDivF;
	u64 ulFreq = (nSourceRate << CLK_DIV_FRAC_BITS) / ulDivider;
	assert (ulFreq <= m_nFreqMax);
#endif

	// set clock divider
	write32 (CLK_DIV_INT (m_Clock), nDivI);
	write32 (CLK_DIV_FRAC (m_Clock), nDivF << (32 - CLK_DIV_FRAC_BITS));

	// set clock parent (aux source only)
	assert (m_nAuxSrc < MaxParents);
	u32 nCtrl = read32 (CLK_CTRL (m_Clock));
	nCtrl &= CLK_CTRL_AUXSRC__MASK;
	nCtrl |= m_nAuxSrc << CLK_CTRL_AUXSRC__SHIFT;
	write32 (CLK_CTRL (m_Clock), nCtrl);

	// enable clock
	write32 (CLK_CTRL (m_Clock), read32 (CLK_CTRL (m_Clock)) | CLK_CTRL_ENABLE__MASK);

	// enable GPIO clock gate
	if (GPIOClock0 <= m_Clock && m_Clock <= GPIOClock2)
	{
		// See: https://forums.raspberrypi.com/viewtopic.php?t=365761#p2194321
		write32 (ARM_GPIO_CLK_BASE,
			 read32 (ARM_GPIO_CLK_BASE) | (1 << (m_Clock - GPIOClock0)));
	}
}

boolean CGPIOClock::StartRate (unsigned nRateHZ)
{
	assert (m_Clock < GPIOClockUnknown);

	assert (nRateHZ > 0);
	if (nRateHZ > m_nFreqMax)
	{
		return FALSE;
	}

	if (m_Source == GPIOClockSourceUnknown)
	{
		// find clock source, which fits best for requested clock rate
		int nFreqDiffMin = nRateHZ;
		unsigned nBestParent = MaxParents;
		TGPIOClockSource BestSource = GPIOClockSourceUnknown;

		for (unsigned i = 0; i < MaxParents; i++)
		{
			TGPIOClockSource Source =
				static_cast<TGPIOClockSource> (s_ParentAux[m_Clock][i]);
			if (Source != GPIOClockSourceNone)
			{
				u64 nSourceRate =
					CMachineInfo::Get ()->GetGPIOClockSourceRate (Source);
				if (nSourceRate >= nRateHZ)
				{
					nSourceRate <<= CLK_DIV_FRAC_BITS;
					u64 ulDivider = (nSourceRate + nRateHZ/2) / nRateHZ;
					u64 ulFreq = nSourceRate / ulDivider;

					s64 lDiff = ulFreq - nRateHZ;
					if (lDiff == 0)
					{
						nBestParent = i;
						BestSource = Source;

						break;
					}

					if (lDiff < 0)
					{
						lDiff = -lDiff;
					}

					if (lDiff < nFreqDiffMin)
					{
						nFreqDiffMin = lDiff;
						nBestParent = i;
						BestSource = Source;
					}
				}
			}
		}

		if (nBestParent < MaxParents)
		{
			m_nAuxSrc = nBestParent;
			m_Source = BestSource;
		}
	}

	assert (m_Source < GPIOClockSourceUnknown);
	u64 nSourceRate = CMachineInfo::Get ()->GetGPIOClockSourceRate (m_Source);
	assert (nSourceRate > 0);

	u64 ulDivider = nSourceRate << CLK_DIV_FRAC_BITS;
	ulDivider = (ulDivider + nRateHZ/2) / nRateHZ;

	u32 nDivI = ulDivider >> CLK_DIV_FRAC_BITS;
	u32 nDivF = ulDivider & ((1 << CLK_DIV_FRAC_BITS) - 1);

	if (1 <= nDivI && nDivI <= m_nDivIntMax)
	{
		Start (nDivI, nDivF);

		return TRUE;
	}

	return FALSE;
}

void CGPIOClock::Stop (void)
{
	assert (m_Clock < GPIOClockUnknown);

	// disable GPIO clock gate
	if (GPIOClock0 <= m_Clock && m_Clock <= GPIOClock2)
	{
		write32 (ARM_GPIO_CLK_BASE,
			 read32 (ARM_GPIO_CLK_BASE) & ~(1 << (m_Clock - GPIOClock0)));
	}

	// disable clock
	write32 (CLK_CTRL (m_Clock), read32 (CLK_CTRL (m_Clock)) & ~CLK_CTRL_ENABLE__MASK);
}

#ifndef NDEBUG

LOGMODULE ("gpioclk");

void CGPIOClock::DumpStatus (boolean bEnabledOnly)
{
	LOGDBG ("CLK ADDR CTRL       DIV  FRAC SEL AUX FL");

	for (unsigned nClock = 0; nClock < GPIOClockUnknown; nClock++)
	{
		u32 nCtrl = read32 (CLK_CTRL (nClock));
		if (   bEnabledOnly
		    && !(nCtrl & CLK_CTRL_ENABLE__MASK))
		{
			continue;
		}

		u32 nDivInt = read32 (CLK_DIV_INT (nClock));
		u32 nDivFrac = read32 (CLK_DIV_FRAC (nClock)) >> (32 - CLK_DIV_FRAC_BITS);
		u32 nSel = read32 (CLK_SEL (nClock));

		LOGDBG ("%02u: %4X %08X %5u %5u %3u %3u %c",
			nClock, CLK_CTRL (nClock) & 0xFFF,
			nCtrl, nDivInt, nDivFrac, nSel,
			(nCtrl & CLK_CTRL_AUXSRC__MASK) >> CLK_CTRL_AUXSRC__SHIFT,
			nCtrl & CLK_CTRL_ENABLE__MASK ? 'e' : ' ');
	}
}

#endif
