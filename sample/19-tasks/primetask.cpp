//
// primetask.cpp
//
// Prime number calculation task
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019  R. Stange <rsta2@o2online.de>
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
#include "primetask.h"

#include <circle/sched/scheduler.h>
#include <circle/string.h>
#include <circle/util.h>

// The CPU is given up after each YIELD_COUNT loop cycles, so that other
// tasks can run. Increase this value for faster prime number calculation,
// decrease it, to give the other tasks more time to run.
#define YIELD_COUNT	1000000

CPrimeTask::CPrimeTask (CScreenDevice *pScreen)
:	m_pScreen (pScreen)
{
}

CPrimeTask::~CPrimeTask (void)
{
}

void CPrimeTask::Run (void)
{
	// Sieve of Eratosthenes
	memset (m_PrimeArray, 0xFF, sizeof m_PrimeArray);	// set all bits

	for (unsigned i = 2; i <= PRIME_MAX_SQRT; i++)
	{
		if (IsPrime (i))
		{
			unsigned nYieldCount = YIELD_COUNT;

			for (unsigned j = i*i; j < PRIME_MAX; j += i)
			{
				NotPrime (j);

				if (--nYieldCount == 0)
				{
					CScheduler::Get ()->Yield ();	// give up CPU

					nYieldCount = YIELD_COUNT;
				}
			}

			CScheduler::Get ()->Yield ();		// give up CPU
		}
	}

	// display largest prime on screen
	for (unsigned i = PRIME_MAX-1; i >= 2; i--)
	{
		if (IsPrime (i))
		{
			CString Message;
			Message.Format ("Largest calculated prime number is %u.\n", i);

			m_pScreen->Write (Message, Message.GetLength ());

			break;
		}
	}

	// task will terminate automatically
}

boolean CPrimeTask::IsPrime (unsigned nNumber)
{
	return m_PrimeArray[nNumber / 32] & (1 << (nNumber % 32)) ? TRUE : FALSE;
}

void CPrimeTask::NotPrime (unsigned nNumber)
{
	m_PrimeArray[nNumber / 32] &= ~(1 << (nNumber % 32));
}
