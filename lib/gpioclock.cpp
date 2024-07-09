//
// gpioclock.cpp
//
// This was mainly posted by joan@raspi-forum:
// 	10MHzClock.c
// 	2014-10-24
// 	Public Domain
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
#include <circle/gpioclock.h>
#include <circle/bcm2835.h>
#include <circle/timer.h>
#include <circle/memio.h>
#include <circle/synchronize.h>
#include <circle/machineinfo.h>
#include <assert.h>

#define CLK_CTL_MASH(x)		((x) << 9)
#define CLK_CTL_BUSY		(1 << 7)
#define CLK_CTL_KILL		(1 << 5)
#define CLK_CTL_ENAB		(1 << 4)
#define CLK_CTL_SRC(x)		((x) << 0)

#define CLK_DIV_DIVI(x)		((x) << 12)
#define CLK_DIV_DIVF(x)		((x) << 0)

CGPIOClock::CGPIOClock (TGPIOClock Clock, TGPIOClockSource Source)
:	m_Clock (Clock),
	m_Source (Source)
{
	assert (m_Source <= GPIOClockSourceUnknown);
}

CGPIOClock::~CGPIOClock (void)
{
	Stop ();
}

boolean CGPIOClock::Start (unsigned nDivI, unsigned nDivF, unsigned nMASH)
{
#ifndef NDEBUG
	static unsigned MinDivI[] = {1, 2, 3, 5};

	assert (nMASH <= 3);
	assert (MinDivI[nMASH] <= nDivI && nDivI <= 4095);
	assert (nDivF <= 4095);
#endif

	unsigned nCtlReg = ARM_CM_BASE + (m_Clock * 8);
	unsigned nDivReg  = nCtlReg + 4;

	Stop ();

	PeripheralEntry ();

	write32 (nDivReg, ARM_CM_PASSWD | CLK_DIV_DIVI (nDivI) | CLK_DIV_DIVF (nDivF));

	CTimer::SimpleusDelay (10);

	assert (m_Source < GPIOClockSourceUnknown);
	write32 (nCtlReg, ARM_CM_PASSWD | CLK_CTL_MASH (nMASH) | CLK_CTL_SRC (m_Source));

	CTimer::SimpleusDelay (10);

	write32 (nCtlReg, read32 (nCtlReg) | ARM_CM_PASSWD | CLK_CTL_ENAB);

	PeripheralExit ();

	return TRUE;
}

boolean CGPIOClock::StartRate (unsigned nRateHZ)
{
	assert (nRateHZ > 0);

	for (unsigned nSourceId = 0; nSourceId <= GPIO_CLOCK_SOURCE_ID_MAX; nSourceId++)
	{
		unsigned nSourceRate = CMachineInfo::Get ()->GetGPIOClockSourceRate (nSourceId);
		if (nSourceRate == GPIO_CLOCK_SOURCE_UNUSED)
		{
			continue;
		}

		unsigned nDivI = nSourceRate / nRateHZ;
		if (   1 <= nDivI && nDivI <= 4095
		    && nRateHZ * nDivI == nSourceRate)
		{
			m_Source = (TGPIOClockSource) nSourceId;

			return Start (nDivI, 0, 0);
		}
	}

	return FALSE;
}

void CGPIOClock::Stop (void)
{
	unsigned nCtlReg = ARM_CM_GP0CTL + (m_Clock * 8);

	PeripheralEntry ();

	write32 (nCtlReg, ARM_CM_PASSWD | CLK_CTL_KILL);
	while (read32 (nCtlReg) & CLK_CTL_BUSY)
	{
		// wait for clock to stop
	}

	PeripheralExit ();
}
