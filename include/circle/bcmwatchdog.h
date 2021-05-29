//
// bcmwatchdog.h
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
#ifndef _circle_bcmwatchdog_h
#define _circle_bcmwatchdog_h

#include <circle/spinlock.h>
#include <circle/types.h>

class CBcmWatchdog	/// Driver for the BCM2835 watchdog device
{
public:
	static const unsigned MaxTimeoutSeconds = 15;

	static const unsigned PartitionDefault = 0;
	static const unsigned PartitionHalt = 63;	///< Halts the system

public:
	CBcmWatchdog (void);
	~CBcmWatchdog (void);

	/// \param nTimeoutSeconds System restarts after this timeout, if not re-triggered
	void Start (unsigned nTimeoutSeconds = MaxTimeoutSeconds);

	void Stop (void);

	/// \brief Immediately restart the system
	/// \param nPartition The partition to be used for restart
	void Restart (unsigned nPartition = PartitionDefault);

	/// \return Is the watchdog currently running?
	boolean IsRunning (void) const;
	/// \return Number of seconds left until restart
	unsigned GetTimeLeft (void) const;

private:
	CSpinLock m_SpinLock;
};

#endif
