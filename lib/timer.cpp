//
// timer.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#include <circle/timer.h>
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/synchronize.h>

#define CLOCKHZ		1000000

void CTimer::SimpleMsDelay (unsigned nMilliSeconds)
{
	if (nMilliSeconds > 0)
	{
		SimpleusDelay (nMilliSeconds * 1000);
	}
}

void CTimer::SimpleusDelay (unsigned nMicroSeconds)
{
	if (nMicroSeconds > 0)
	{
		unsigned nTicks = nMicroSeconds * (CLOCKHZ / 1000000);

		DataMemBarrier ();

		unsigned nStartTicks = read32 (ARM_SYSTIMER_CLO);
		while (read32 (ARM_SYSTIMER_CLO) - nStartTicks < nTicks)
		{
			// do nothing
		}
	}
}
