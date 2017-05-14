//
// recorder.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
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
#ifndef _recorder_h
#define _recorder_h

#include <circle/gpiopin.h>
#include <circle/types.h>
#include "scopeconfig.h"

class CRecorder
{
public:
	CRecorder (unsigned nCh1Pin, unsigned nCh2Pin, unsigned nCh3Pin, unsigned nCh4Pin,
		   CScopeConfig *pConfig);
	~CRecorder (void);

	boolean Initialize (void);

	unsigned GetPin (unsigned nChannel) const;	// returns GPIO pin number for channel

	// must be called before Run()
	void SetMemoryDepth (unsigned nMemoryDepth);
	void SetTrigger (u32 nMask, u32 nLevel);
	void SetDelayCount (u32 nDelayCount);

	boolean Run (void);
	unsigned GetRuntime (void) const;		// microseconds, 0 if did not run before

	unsigned GetSample (unsigned nChannel, unsigned nOffset) const;
#define LOW	0
#define HIGH	1

private:
	unsigned Calibrate (unsigned nWantedRateKHz, unsigned *pActualRateKHz);	// returns delay count
	unsigned CalibrateFine (unsigned nDelayCount, unsigned nWantedRuntime);	// returns delay count

	unsigned RunCalibrate (unsigned nDelayCount);		// returns run time

private:
	unsigned m_nCh1Pin;
	unsigned m_nCh2Pin;
	unsigned m_nCh3Pin;
	unsigned m_nCh4Pin;
	CScopeConfig *m_pConfig;

	CGPIOPin m_Ch1;
	CGPIOPin m_Ch2;
	CGPIOPin m_Ch3;
	CGPIOPin m_Ch4;

	u32 *m_pSamples;

	unsigned m_nCPUClockKHz;

	u32 m_nMemoryDepth;
	u32 m_nTriggerMask;
	u32 m_nTriggerLevel;
	u32 m_nDelayCount;

	u32 m_nRuntime;

	static unsigned s_nWantedRateKHz[];
};

#endif
