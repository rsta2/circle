//
// bcmrandom200.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2019  R. Stange <rsta2@o2online.de>
//
// This file contains code taken from Linux:
//	drivers/char/hw_random/iproc-rng200.c
//	iProc RNG200 Random Number Generator driver
//	Copyright (C) 2015 Broadcom Corporation
//	Licensed under GPLv2
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
#include <circle/bcmrandom.h>
#include <circle/bcm2711.h>
#include <circle/memio.h>

#define RNG_CTRL					(ARM_HW_RNG200_BASE + 0x00)
	#define RNG_CTRL_RNG_RBGEN__MASK			0x00001FFF
	#define RNG_CTRL_RNG_DIV_CTRL__SHIFT			13
#define RNG_TOTAL_BIT_COUNT				(ARM_HW_RNG200_BASE + 0x0C)
#define RNG_TOTAL_BIT_COUNT_THRESHOLD			(ARM_HW_RNG200_BASE + 0x10)
#define RNG_FIFO_DATA					(ARM_HW_RNG200_BASE + 0x20)
#define RNG_FIFO_COUNT					(ARM_HW_RNG200_BASE + 0x24)
	#define RNG_FIFO_COUNT_RNG_FIFO_COUNT__MASK		0x000000FF
	#define RNG_FIFO_COUNT_RNG_FIFO_THRESHOLD__SHIFT	8

CSpinLock CBcmRandomNumberGenerator::s_SpinLock (TASK_LEVEL);

boolean CBcmRandomNumberGenerator::s_bInitialized = FALSE;

CBcmRandomNumberGenerator::CBcmRandomNumberGenerator (void)
{
	s_SpinLock.Acquire ();

	if (!s_bInitialized)
	{
		s_bInitialized = TRUE;

		// initial numbers generated are "less random" so will be discarded
		write32 (RNG_TOTAL_BIT_COUNT_THRESHOLD, 0x40000);

		// min FIFO count to generate full interrupt
		write32 (RNG_FIFO_COUNT, 2 << RNG_FIFO_COUNT_RNG_FIFO_THRESHOLD__SHIFT);

		// enable the RNG, 1 MHz sample rate
		write32 (RNG_CTRL,   (3 << RNG_CTRL_RNG_DIV_CTRL__SHIFT)
				   | RNG_CTRL_RNG_RBGEN__MASK);

		// ensure warm up period has elapsed
		while (read32 (RNG_TOTAL_BIT_COUNT) <= 16)
		{
			// just wait
		}
	}

	s_SpinLock.Release ();
}

CBcmRandomNumberGenerator::~CBcmRandomNumberGenerator (void)
{
}

u32 CBcmRandomNumberGenerator::GetNumber (void)
{
	s_SpinLock.Acquire ();

	// ensure FIFO is not empty
	while ((read32 (RNG_FIFO_COUNT) & RNG_FIFO_COUNT_RNG_FIFO_COUNT__MASK) == 0)
	{
		// just wait
	}

	u32 nResult = read32 (RNG_FIFO_DATA);

	s_SpinLock.Release ();

	return nResult;
}
