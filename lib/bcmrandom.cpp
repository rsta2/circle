//
// bcmrandom.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2017  R. Stange <rsta2@o2online.de>
// 
// This file contains code taken from Linux:
//	drivers/char/hw_random/bcm2835-rng.c
//	Copyright (c) 2010-2012 Broadcom. All rights reserved.
//	Copyright (c) 2013 Lubomir Rintel
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
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/synchronize.h>

// the initial numbers generated are "less random" so will be discarded
#define RNG_WARMUP_COUNT	0x40000

CSpinLock CBcmRandomNumberGenerator::s_SpinLock (TASK_LEVEL);

boolean CBcmRandomNumberGenerator::s_bInitialized = FALSE;

CBcmRandomNumberGenerator::CBcmRandomNumberGenerator (void)
{
	s_SpinLock.Acquire ();

	if (!s_bInitialized)
	{
		s_bInitialized = TRUE;

		PeripheralEntry ();

		write32 (ARM_HW_RNG_STATUS, RNG_WARMUP_COUNT);
		write32 (ARM_HW_RNG_CTRL, ARM_HW_RNG_CTRL_EN);

		PeripheralExit ();
	}

	s_SpinLock.Release ();
}

CBcmRandomNumberGenerator::~CBcmRandomNumberGenerator (void)
{
}

u32 CBcmRandomNumberGenerator::GetNumber (void)
{
	s_SpinLock.Acquire ();

	PeripheralEntry ();

	while ((read32 (ARM_HW_RNG_STATUS) >> 24) == 0)
	{
		// just wait
	}
	
	u32 nResult = read32 (ARM_HW_RNG_DATA);

	PeripheralExit ();

	s_SpinLock.Release ();

	return nResult;
}
