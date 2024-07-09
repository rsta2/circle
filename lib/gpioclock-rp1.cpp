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
#include <circle/timer.h>
#include <circle/machineinfo.h>
#include <circle/macros.h>
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

#define PLL_AUDIO_CORE_CS		(ARM_GPIO_CLK_BASE + 0x0C000)
	#define PLL_CS_LOCK		BIT(31)
	#define PLL_CS_REFDIV_SHIFT	0
#define PLL_AUDIO_CORE_PWR		(ARM_GPIO_CLK_BASE + 0x0C004)
	#define PLL_PWR_PD		BIT(0)
	#define PLL_PWR_DACPD		BIT(1)
	#define PLL_PWR_DSMPD		BIT(2)
	#define PLL_PWR_POSTDIVPD	BIT(3)
	#define PLL_PWR_4PHASEPD	BIT(4)
	#define PLL_PWR_VCOPD		BIT(5)
	#define PLL_PWR_MASK		0x3FU
#define PLL_AUDIO_CORE_FBDIV_INT	(ARM_GPIO_CLK_BASE + 0x0C008)
#define PLL_AUDIO_CORE_FBDIV_FRAC	(ARM_GPIO_CLK_BASE + 0x0C00C)
#define PLL_AUDIO_PRIM			(ARM_GPIO_CLK_BASE + 0x0C010)
	#define PLL_PRIM_DIV1_SHIFT	16
	#define PLL_PRIM_DIV1_MASK	0x00070000U
	#define PLL_PRIM_DIV2_SHIFT	12
	#define PLL_PRIM_DIV2_MASK	0x00007000U
#define PLL_AUDIO_SEC			(ARM_GPIO_CLK_BASE + 0x0C014)
	#define PLL_SEC_DIV_SHIFT	8
	#define PLL_SEC_DIV_WIDTH	5
	#define PLL_SEC_DIV_MASK	0x1F00U
	#define PLL_SEC_RST		BIT(16)
	#define PLL_SEC_IMPL		BIT(31)

#define LOCK_TIMEOUT_US			100000

#define DIV_ROUND_UP(n, d)		(((n) + (d) - 1) / (d))
#define DIV_ROUND_CLOSEST(n, d)		(((n) + (d) / 2) / (d))
#define ABS_DIFF(n, m)			((n) > (m) ? (n) - (m) : (m) - (n))

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
	{GPIOClockSourceXOscillator, GPIOClockSourcePLLAudio}, // I2S

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

const CGPIOClock::TAudioClock CGPIOClock::s_AudioClock[] =
{
//	pll_audio_core  pll_audio	clk_i2s		sample_rate

	{1536000000,	102400000,	512000},	// 8000
	{801792000,	100224000,	705600},	// 11025
	{1536000000,	102400000,	1024000},	// 16000
	{801792000,	200448000,	1411200},	// 22050
	{1536000000,	102400000,	2048000},	// 32000
	{801792000,	200448000,	2822400},	// 44100
	{801792000,	267264000,	3072000},	// 48000
	{801792000,	400896000,	5644800},	// 88200
	{1536000000,	153600000,	6144000},	// 96000
	{801792000,	801792000,	11289600},	// 176400
	{1536000000,	61440000,	12288000},	// 192000

	{0,		0,		0}
};

LOGMODULE ("gpioclk");

CGPIOClock::CGPIOClock (TGPIOClock Clock, TGPIOClockSource Source)
:	m_Clock (Clock),
	m_Source (Source),
	m_nDivIntMax (0),
	m_nFreqMax (0),
	m_nAuxSrc (MaxParents),
	m_nRateHZ (0)
{
	switch (Clock)
	{
	case GPIOClockPWM0:
	case GPIOClockPWM1:
		m_nDivIntMax = DIV_INT_16BIT_MAX;
		m_bHasFrac = TRUE;
		m_nFreqMax = 76800000;
		break;

	case GPIOClockAudioIn:
		m_nDivIntMax = DIV_INT_8BIT_MAX;
		m_bHasFrac = FALSE;
		m_nFreqMax = 76800000;
		break;

	case GPIOClockAudioOut:
		m_nDivIntMax = DIV_INT_8BIT_MAX;
		m_bHasFrac = FALSE;
		m_nFreqMax = 153600000;
		break;

	case GPIOClockI2S:
		m_nDivIntMax = DIV_INT_8BIT_MAX;
		m_bHasFrac = FALSE;
		m_nFreqMax = 50000000;
		break;

	case GPIOClock0:
	case GPIOClock1:
	case GPIOClock2:
		m_nDivIntMax = DIV_INT_16BIT_MAX;
		m_bHasFrac = TRUE;
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

boolean CGPIOClock::Start (unsigned nDivI, unsigned nDivF)
{
	assert (m_Clock < GPIOClockUnknown);
	assert (1 <= nDivI && nDivI <= m_nDivIntMax);
	assert (m_bHasFrac || !nDivF);
	assert (nDivF <= (1 << CLK_DIV_FRAC_BITS)-1);

	Stop ();

	assert (m_Source < GPIOClockSourceUnknown);
	u64 nSourceRate = GetSourceRate (m_Source, m_nRateHZ);
	if (!nSourceRate)
	{
		LOGERR ("Clock rate is not supported (%u)", m_nRateHZ);

		return FALSE;
	}

	// check maximum frequency
	u64 ulDivider = ((u64) nDivI << CLK_DIV_FRAC_BITS) | (m_bHasFrac ? nDivF : 0);
	u64 ulFreq = (nSourceRate << CLK_DIV_FRAC_BITS) / ulDivider;
	if (ulFreq > m_nFreqMax)
	{
		LOGERR ("Clock rate is too high (%lu, max %u)", ulFreq, m_nFreqMax);

		return FALSE;
	}

	// set clock divider
	write32 (CLK_DIV_INT (m_Clock), nDivI);

	if (m_bHasFrac)
	{
		write32 (CLK_DIV_FRAC (m_Clock), nDivF << (32 - CLK_DIV_FRAC_BITS));
	}

	// set clock parent (aux source only)
	assert (m_nAuxSrc < MaxParents);
	u32 nCtrl = read32 (CLK_CTRL (m_Clock));
	nCtrl &= CLK_CTRL_AUXSRC__MASK;
	nCtrl |= m_nAuxSrc << CLK_CTRL_AUXSRC__SHIFT;
	write32 (CLK_CTRL (m_Clock), nCtrl);

	// enable clock
	write32 (CLK_CTRL (m_Clock), read32 (CLK_CTRL (m_Clock)) | CLK_CTRL_ENABLE__MASK);

	if (m_Source == GPIOClockSourcePLLAudio)
	{
		unsigned nPLLRate = GetSourceRate (GPIOClockSourcePLLAudio, m_nRateHZ);
		unsigned nPLLCoreRate = GetSourceRate (GPIOClockSourcePLLAudioCore, m_nRateHZ);

		if (!EnablePLLAudio (nPLLRate, nPLLCoreRate))
		{
			LOGERR ("Cannot enable pll_audio (%u, %u)", nPLLRate, nPLLCoreRate);

			return FALSE;
		}

		if (!EnablePLLAudioCore (nPLLCoreRate))
		{
			LOGERR ("Cannot enable pll_audio_core (%u)", nPLLCoreRate);

			return FALSE;
		}
	}

	// enable GPIO clock gate
	if (GPIOClock0 <= m_Clock && m_Clock <= GPIOClock2)
	{
		// See: https://forums.raspberrypi.com/viewtopic.php?t=365761#p2194321
		write32 (ARM_GPIO_CLK_BASE,
			 read32 (ARM_GPIO_CLK_BASE) | (1 << (m_Clock - GPIOClock0)));
	}

	return TRUE;
}

boolean CGPIOClock::StartRate (unsigned nRateHZ)
{
	assert (m_Clock < GPIOClockUnknown);

	assert (nRateHZ > 0);
	if (nRateHZ > m_nFreqMax)
	{
		return FALSE;
	}

	m_nRateHZ = nRateHZ;

	if (m_Source == GPIOClockSourceUnknown)
	{
		// find clock source, which fits best for requested clock rate
		unsigned nFreqDiffMin = nRateHZ;
		unsigned nBestParent = MaxParents;
		TGPIOClockSource BestSource = GPIOClockSourceUnknown;

		for (unsigned i = 0; i < MaxParents; i++)
		{
			TGPIOClockSource Source =
				static_cast<TGPIOClockSource> (s_ParentAux[m_Clock][i]);
			if (Source != GPIOClockSourceNone)
			{
				u64 nSourceRate = GetSourceRate (Source, nRateHZ);
				if (nSourceRate >= nRateHZ)
				{
					nSourceRate <<= CLK_DIV_FRAC_BITS;
					u64 ulDivider = DIV_ROUND_CLOSEST (nSourceRate, nRateHZ);
					if (!m_bHasFrac)
					{
						ulDivider &= ~((1 << CLK_DIV_FRAC_BITS) - 1);
					}

					u64 ulFreq = nSourceRate / ulDivider;

					u64 ulDiff = ABS_DIFF (ulFreq, nRateHZ);
					if (!ulDiff)
					{
						nBestParent = i;
						BestSource = Source;

						break;
					}
					else if (ulDiff < nFreqDiffMin)
					{
						nFreqDiffMin = ulDiff;
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
	u64 nSourceRate = GetSourceRate (m_Source, nRateHZ);
	if (!nSourceRate)
	{
		LOGERR ("Clock rate is not supported (%u)", nRateHZ);

		return FALSE;
	}

	u64 ulDivider = nSourceRate << CLK_DIV_FRAC_BITS;
	ulDivider = (ulDivider + nRateHZ/2) / nRateHZ;

	u32 nDivI = ulDivider >> CLK_DIV_FRAC_BITS;
	u32 nDivF = ulDivider & ((1 << CLK_DIV_FRAC_BITS) - 1);
	if (!m_bHasFrac)
	{
		nDivF = 0;
	}

	if (1 <= nDivI && nDivI <= m_nDivIntMax)
	{
		return Start (nDivI, nDivF);
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

	if (   m_Source == GPIOClockSourcePLLAudio
	    && read32 (PLL_AUDIO_CORE_CS) & PLL_CS_LOCK)
	{
		// reset to a known state
		write32 (PLL_AUDIO_CORE_PWR, PLL_PWR_MASK);
		write32 (PLL_AUDIO_CORE_FBDIV_INT, 20);
		write32 (PLL_AUDIO_CORE_FBDIV_FRAC, 0);
		write32 (PLL_AUDIO_CORE_CS, 1 << PLL_CS_REFDIV_SHIFT);
	}
}

boolean CGPIOClock::EnablePLLAudioCore (unsigned long ulRate)
{
	// is PLL already on and locked?
	if (read32 (PLL_AUDIO_CORE_CS) & PLL_CS_LOCK)
	{
		// reset to a known state
		write32 (PLL_AUDIO_CORE_PWR, PLL_PWR_MASK);
		write32 (PLL_AUDIO_CORE_FBDIV_INT, 20);
		write32 (PLL_AUDIO_CORE_FBDIV_FRAC, 0);
		write32 (PLL_AUDIO_CORE_CS, 1 << PLL_CS_REFDIV_SHIFT);
	}

	unsigned long ulParentRate = GetSourceRate (GPIOClockSourceXOscillator);

	u32 fbdiv_int, fbdiv_frac;
	GetPLLCoreDivider (ulRate, ulParentRate, &fbdiv_int, &fbdiv_frac);

	write32 (PLL_AUDIO_CORE_FBDIV_INT, fbdiv_int);
	write32 (PLL_AUDIO_CORE_FBDIV_FRAC, fbdiv_frac);

	// power on
	write32 (PLL_AUDIO_CORE_PWR, fbdiv_frac ? 0 : PLL_PWR_DSMPD);

	// don't need to divide ref unless parent rate > (output freq / 16)
	assert (ulParentRate <= (ulRate / 16));
	write32 (PLL_AUDIO_CORE_CS, read32 (PLL_AUDIO_CORE_CS) | (1 << PLL_CS_REFDIV_SHIFT));

	// wait for the PLL to lock
	unsigned nStartTicks = CTimer::GetClockTicks ();
	while (!(read32 (PLL_AUDIO_CORE_CS) & PLL_CS_LOCK))
	{
		if (CTimer::GetClockTicks () - nStartTicks >= LOCK_TIMEOUT_US * (CLOCKHZ / 1000000))
		{
			LOGERR ("Cannot lock PLL (%lu)", ulRate);

			return FALSE;
		}

		CTimer::SimpleusDelay (100);
	}

	return TRUE;
}

boolean CGPIOClock::EnablePLLAudio (unsigned long ulRate, unsigned long ulParentRate)
{
	u32 prim_div1, prim_div2;
	GetPLLDividers (ulRate, ulParentRate, &prim_div1, &prim_div2);

	u32 prim = read32 (PLL_AUDIO_PRIM);

	prim &= ~PLL_PRIM_DIV1_MASK;
	prim |= (prim_div1 << PLL_PRIM_DIV1_SHIFT) & PLL_PRIM_DIV1_MASK;

	prim &= ~PLL_PRIM_DIV2_MASK;
	prim |= (prim_div2 << PLL_PRIM_DIV2_SHIFT) & PLL_PRIM_DIV2_MASK;

	write32 (PLL_AUDIO_PRIM, prim);

	return TRUE;
}

unsigned long CGPIOClock::GetPLLCoreDivider (unsigned long rate, unsigned long parent_rate,
					     u32 *div_int, u32 *div_frac)
{
	unsigned long calc_rate;
	u32 fbdiv_int, fbdiv_frac;
	u64 div_fp64; /* 32.32 fixed point fraction. */

	/* Factor of reference clock to VCO frequency. */
	div_fp64 = (u64)(rate) << 32;
	div_fp64 = DIV_ROUND_CLOSEST(div_fp64, parent_rate);

	/* Round the fractional component at 24 bits. */
	div_fp64 += 1 << (32 - 24 - 1);

	fbdiv_int = div_fp64 >> 32;
	fbdiv_frac = (div_fp64 >> (32 - 24)) & 0xffffff;

	calc_rate = ((u64)parent_rate * (((u64)fbdiv_int << 24) + fbdiv_frac) + (1 << 23)) >> 24;

	*div_int = fbdiv_int;
	*div_frac = fbdiv_frac;

	return calc_rate;
}

void CGPIOClock::GetPLLDividers (unsigned long rate, unsigned long parent_rate,
				 u32 *divider1, u32 *divider2)
{
	unsigned int div1, div2;
	unsigned int best_div1 = 7, best_div2 = 7;
	unsigned long best_rate_diff =
		ABS_DIFF(DIV_ROUND_CLOSEST(parent_rate, best_div1 * best_div2), rate);
	unsigned long rate_diff, calc_rate;

	for (div1 = 1; div1 <= 7; div1++) {
		for (div2 = 1; div2 <= div1; div2++) {
			calc_rate = DIV_ROUND_CLOSEST(parent_rate, div1 * div2);
			rate_diff = ABS_DIFF(calc_rate, rate);

			if (calc_rate == rate) {
				best_div1 = div1;
				best_div2 = div2;
				goto done;
			} else if (rate_diff < best_rate_diff) {
				best_div1 = div1;
				best_div2 = div2;
				best_rate_diff = rate_diff;
			}
		}
	}

done:
	*divider1 = best_div1;
	*divider2 = best_div2;
}

unsigned CGPIOClock::GetSourceRate (unsigned nSourceId, unsigned nClockI2SRate)
{
	if (   nSourceId != GPIOClockSourcePLLAudioCore
	    && nSourceId != GPIOClockSourcePLLAudio)
	{
		return CMachineInfo::Get ()->GetGPIOClockSourceRate (nSourceId);
	}

	assert (nClockI2SRate);
	for (unsigned i = 0; s_AudioClock[i].ClockI2SRate; i++)
	{
		if (s_AudioClock[i].ClockI2SRate == nClockI2SRate)
		{
			return   nSourceId == GPIOClockSourcePLLAudioCore
			       ? s_AudioClock[i].PLLAudioCoreRate
			       : s_AudioClock[i].PLLAudioRate;
		}
	}

	return 0;
}

#ifndef NDEBUG

void CGPIOClock::DumpStatus (boolean bEnabledOnly)
{
	LOGDBG ("PLL   CS       PWR INT     FRAC PRIM");

	u32 nPrim = read32 (PLL_AUDIO_PRIM);

	LOGDBG ("AUDIO %08X  %02X  %2u %8u %u*%u",
		read32 (PLL_AUDIO_CORE_CS),
		read32 (PLL_AUDIO_CORE_PWR),
		read32 (PLL_AUDIO_CORE_FBDIV_INT),
		read32 (PLL_AUDIO_CORE_FBDIV_FRAC),
		(nPrim & PLL_PRIM_DIV1_MASK) >> PLL_PRIM_DIV1_SHIFT,
		(nPrim & PLL_PRIM_DIV2_MASK) >> PLL_PRIM_DIV2_SHIFT);

	LOGDBG ("CLK ADDR CTRL       INT  FRAC SEL AUX FL");

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
