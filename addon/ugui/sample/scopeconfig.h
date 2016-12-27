//
// scopeconfig.h
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
#ifndef _scopeconfig_h
#define _scopeconfig_h

#include <circle/ptrarray.h>

#define MAX_MEMORY_DEPTH	100000
#define CALIBRATE_DEPTH		20000
#define MAX_RUNTIME_SECS	1

#define CHANS	4

#define CH(n)	(1 << ((n)-1))
#define CH1	CH(1)
#define CH2	CH(2)
#define CH3	CH(3)
#define CH4	CH(4)

class CScopeConfig
{
public:
	CScopeConfig (void);
	~CScopeConfig (void);

	// must be called with increasing sample rate
	void AddParamSet (unsigned nWantedRateKHz, unsigned nActualRateKHz, unsigned nDelayCount);

	unsigned GetParamSetCount (void) const;

	void SelectParamSet (unsigned nIndex);		// 0-based index
	unsigned GetWantedRate (void) const;		// in KHz
	unsigned GetActualRate (void) const;		// in KHz
	unsigned GetDelayCount (void) const;
	unsigned GetMemoryDepth (void) const;
	unsigned GetChartScale (void) const;		// in microseconds

	unsigned GetActualRate (unsigned nIndex) const;	// in KHz, 0-based index

private:
	CPtrArray m_Table;

	unsigned m_nSelectedIndex;
};

#endif
