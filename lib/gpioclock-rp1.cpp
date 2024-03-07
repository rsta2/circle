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
#include <circle/timer.h>
#include <circle/memio.h>
#include <circle/logger.h>
#include <circle/machineinfo.h>
#include <assert.h>

#define CLK_CTRL(clk)			(ARM_GPIO_CLK_BASE + 0x10 + (clk)*0x10 + 0x04)
	#define CLK_CTRL_ENABLE__MASK	BIT(11)
	#define CLK_CTRL_AUXSRC__SHIFT	5
	#define CLK_CTRL_AUXSRC__MASK	(0x0F << 5)
	#define CLK_CTRL_SRC__SHIFT	0
#define CLK_DIV_INT(clk)		(ARM_GPIO_CLK_BASE + 0x10 + (clk)*0x10 + 0x08)
#define CLK_DIV_FRAC(clk)		(ARM_GPIO_CLK_BASE + 0x10 + (clk)*0x10 + 0x0C)
	#define CLK_DIV_FRAC_BITS	16
#define CLK_SEL(clk)			(ARM_GPIO_CLK_BASE + 0x10 + (clk)*0x10 + 0x10)

CGPIOClock::CGPIOClock (TGPIOClock Clock)
:	m_Clock (Clock)
{
}

CGPIOClock::~CGPIOClock (void)
{
	Stop ();
}

void CGPIOClock::Start (unsigned nDivI, unsigned nDivF)
{
	assert (nDivI <= 0xFFFF);
	assert (nDivF <= 0xFFFF);

	Stop ();

	// set clock divider
	write32 (CLK_DIV_INT (m_Clock), nDivI);
	write32 (CLK_DIV_FRAC (m_Clock), nDivF << 16);

	CTimer::SimpleusDelay (10);

	// TODO: set clock parent

	// enable clock
	write32 (CLK_CTRL (m_Clock), read32 (CLK_CTRL (m_Clock)) | CLK_CTRL_ENABLE__MASK);

	// enable GPIO clock gate
	if (m_Clock == GPIOClock0)
	{
		CTimer::SimpleusDelay (10);

		// See: https://forums.raspberrypi.com/viewtopic.php?t=365761#p2194321
		write32 (ARM_GPIO_CLK_BASE, 1);
	}
}

// TODO: support fractional divider
boolean CGPIOClock::StartRate (unsigned nRateHZ)
{
	assert (nRateHZ > 0);

	unsigned nSourceRate =
		CMachineInfo::Get ()->GetGPIOClockSourceRate (GPIOClockSourceOscillator);

	unsigned nDivI = nSourceRate / nRateHZ;
	if (   1 <= nDivI && nDivI <= 0xFFFF
		&& nRateHZ * nDivI == nSourceRate)
	{
		Start (nDivI, 0);

		return TRUE;
	}

	return FALSE;
}

void CGPIOClock::Stop (void)
{
	// disable GPIO clock gate
	if (m_Clock == GPIOClock0)
	{
		write32 (ARM_GPIO_CLK_BASE, 0);
	}

	// disable clock
	write32 (CLK_CTRL (m_Clock), read32 (CLK_CTRL (m_Clock)) & ~CLK_CTRL_ENABLE__MASK);

	CTimer::SimpleusDelay (10);
}

#ifndef NDEBUG

LOGMODULE ("gpioclk");

void CGPIOClock::DumpStatus (boolean bEnabledOnly)
{
	LOGDBG ("CLK ADDR CTRL       DIV  FRAC SEL  FL");

	for (unsigned nClock = 0; nClock < GPIOClockUnknown; nClock++)
	{
		u32 nCtrl = read32 (CLK_CTRL (nClock));
		if (   bEnabledOnly
		    && !(nCtrl & CLK_CTRL_ENABLE__MASK))
		{
			continue;
		}

		u32 nDivInt = read32 (CLK_DIV_INT (nClock));
		u32 nDivFrac = read32 (CLK_DIV_FRAC (nClock)) >> 16;
		u32 nSel = read32 (CLK_SEL (nClock));

		LOGDBG ("%02u: %4X %08X %5u %5u %u(%u) %c",
			nClock, CLK_CTRL (nClock) & 0xFFF,
			nCtrl, nDivInt, nDivFrac, nSel,
			(nCtrl & CLK_CTRL_AUXSRC__MASK) >> CLK_CTRL_AUXSRC__SHIFT,
			nCtrl & CLK_CTRL_ENABLE__MASK ? 'e' : ' ');
	}
}

#endif
