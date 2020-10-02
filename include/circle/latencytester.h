//
// latencytester.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_latencytester_h
#define _circle_latencytester_h

#include <circle/interrupt.h>
#include <circle/spinlock.h>
#include <circle/types.h>

/// \note CLatencyTester blocks the system timer 1, which is used by the class CUserTimer too.

class CLatencyTester		/// Measures the IRQ latency of the running code
{
public:
	CLatencyTester (CInterruptSystem *pInterruptSystem);
	~CLatencyTester (void);

	/// \brief Start measurement
	/// \param nSampleRateHZ Sample rate in Hz
	void Start (unsigned nSampleRateHZ);
	/// \brief Stop measurement
	void Stop (void);

	/// \return Minimum IRQ latency in microseconds
	unsigned GetMin (void) const;
	/// \return Maximum IRQ latency in microseconds
	unsigned GetMax (void) const;
	/// \return Average IRQ latency in microseconds
	unsigned GetAvg (void);

	/// \brief Dump results to logger
	void Dump (void);

private:
	void InterruptHandler (void);
	static void InterruptStub (void *pParam);

private:
	CInterruptSystem *m_pInterruptSystem;

	boolean m_bRunning;
	unsigned m_nWantedDelay;

	unsigned m_nMinDelay;
	unsigned m_nMaxDelay;

	unsigned m_nDelayAccu;
	unsigned m_nSamples;
	boolean  m_bOverflow;

	CSpinLock m_SpinLock;
};

#endif
