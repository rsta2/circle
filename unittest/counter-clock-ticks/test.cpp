// SPDX-License-Identifier: GPL-3.0-or-later
#include <cassert>
#include <cstdio>

using u64 = unsigned long long;

#include "defines.inc"

struct CTimer
{
#include "function.inc"
};

// The exact clock ticks of a count, cut to 64 bits as a u64 holds them
static u64 Exact (u64 nCount, u64 nFrequency)
{
	return (u64) ((unsigned __int128) nCount * CLOCKHZ / nFrequency);
}

int main ()
{
	// the first count whose product with CLOCKHZ leaves 64 bits
	const u64 nWrapCount = (u64) -1 / CLOCKHZ + 1;

	const u64 Frequencies[] = {19200000, 54000000, 32768, CLOCKHZ, (u64) -1 / CLOCKHZ};
	for (u64 nFrequency : Frequencies)
	{
		const u64 Counts[] = {0, nFrequency - 1, nFrequency, nWrapCount - 1, nWrapCount,
				      nWrapCount + 1, 2 * nWrapCount, (u64) -1};
		for (u64 nCount : Counts)
		{
			assert (CTimer::CounterToClockTicks (nCount, nFrequency) == Exact (nCount, nFrequency));
		}
	}

	std::printf ("Counter to clock ticks passed\n");
}
