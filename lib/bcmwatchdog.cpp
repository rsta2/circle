//
// bcmwatchdog.cpp
//
// This is based on:
//	linux/drivers/watchdog/bcm2835_wdt.c
//	Watchdog driver for Broadcom BCM2835
//	Copyright (C) 2013 Lubomir Rintel <lkundrak@v3.sk>
//	Licensed under GPL-2.0+
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2021  R. Stange <rsta2@o2online.de>
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
#include <circle/bcmwatchdog.h>
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/macros.h>

CBcmWatchdog::CBcmWatchdog (void)
{
}

CBcmWatchdog::~CBcmWatchdog (void)
{
}

void CBcmWatchdog::Start (unsigned nTimeoutSeconds)
{
	if (nTimeoutSeconds > MaxTimeoutSeconds)
	{
		nTimeoutSeconds = MaxTimeoutSeconds;
	}

	m_SpinLock.Acquire ();

	write32 (ARM_PM_WDOG, ARM_PM_PASSWD | ((nTimeoutSeconds << 16) & ARM_PM_WDOG_TIME));

	write32 (ARM_PM_RSTC,   ARM_PM_PASSWD | ARM_PM_RSTC_REBOOT
			      | (read32 (ARM_PM_RSTC) & ARM_PM_RSTC_CLEAR));

	m_SpinLock.Release ();
}

void CBcmWatchdog::Stop (void)
{
	write32 (ARM_PM_RSTC, ARM_PM_PASSWD | ARM_PM_RSTC_RESET);
}

/*
 * The Raspberry Pi firmware uses the RSTS register to know which partiton
 * to boot from. The partiton value is spread into bits 0, 2, 4, 6, 8, 10.
 * Partiton 63 is a special partition used by the firmware to indicate halt.
 */
void CBcmWatchdog::Restart (unsigned nPartition)
{
	u32 nRSTS =    (nPartition & BIT(0))
		    | ((nPartition & BIT(1)) << 1)
		    | ((nPartition & BIT(2)) << 2)
		    | ((nPartition & BIT(3)) << 3)
		    | ((nPartition & BIT(4)) << 4)
		    | ((nPartition & BIT(5)) << 5);

	write32 (ARM_PM_RSTS,   ARM_PM_PASSWD | nRSTS
			      | (read32 (ARM_PM_RSTS) & ARM_PM_RSTS_PART_CLEAR));

	write32 (ARM_PM_WDOG, ARM_PM_PASSWD | 10);	// about 150us timeout

	write32 (ARM_PM_RSTC,   ARM_PM_PASSWD | ARM_PM_RSTC_REBOOT
			      | (read32 (ARM_PM_RSTC) & ARM_PM_RSTC_CLEAR));

	for (;;);
}

boolean CBcmWatchdog::IsRunning (void) const
{
	return !!(read32 (ARM_PM_RSTC) & ARM_PM_RSTC_REBOOT);
}

unsigned CBcmWatchdog::GetTimeLeft (void) const
{
	return (read32 (ARM_PM_WDOG) & ARM_PM_WDOG_TIME) >> 16;
}
