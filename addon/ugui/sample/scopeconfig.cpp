//
// scopeconfig.cpp
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
#include "scopeconfig.h"
#include <assert.h>

struct TParamSet
{
	unsigned nWantedRateKHz;
	unsigned nActualRateKHz;
	unsigned nDelayCount;
	unsigned nMemoryDepth;
	unsigned nChartScale;
};

CScopeConfig::CScopeConfig (void)
:	m_nSelectedIndex (0)
{
}

CScopeConfig::~CScopeConfig (void)
{
}

void CScopeConfig::AddParamSet (unsigned nWantedRateKHz, unsigned nActualRateKHz, unsigned nDelayCount)
{
	unsigned nMemoryDepth =  MAX_RUNTIME_SECS
			       * 1000			// KHz
			       * nActualRateKHz;
	if (nMemoryDepth > MAX_MEMORY_DEPTH)
	{
		nMemoryDepth = MAX_MEMORY_DEPTH;
	}

	unsigned nChartScale =   100			// chart pixels
			       * 1000000		// microseconds
			       / 1000			// KHz
			       / nWantedRateKHz;
	nChartScale = (nChartScale+5) / 10 * 10;
	if (nChartScale < 10)
	{
		nChartScale = 10;
	}

	TParamSet *pSet = new TParamSet;
	assert (pSet != 0);

	pSet->nWantedRateKHz = nWantedRateKHz;
	pSet->nActualRateKHz = nActualRateKHz;
	pSet->nDelayCount    = nDelayCount;
	pSet->nMemoryDepth   = nMemoryDepth;
	pSet->nChartScale    = nChartScale;

	m_Table.Append (pSet);
}

unsigned CScopeConfig::GetParamSetCount (void) const
{
	return m_Table.GetCount ();
}

void CScopeConfig::SelectParamSet (unsigned nIndex)
{
	assert (nIndex < m_Table.GetCount ());
	m_nSelectedIndex = nIndex;
}

unsigned CScopeConfig::GetWantedRate (void) const
{
	return ((TParamSet *) m_Table[m_nSelectedIndex])->nWantedRateKHz;
}

unsigned CScopeConfig::GetActualRate (void) const
{
	return ((TParamSet *) m_Table[m_nSelectedIndex])->nActualRateKHz;
}

unsigned CScopeConfig::GetDelayCount (void) const
{
	return ((TParamSet *) m_Table[m_nSelectedIndex])->nDelayCount;
}

unsigned CScopeConfig::GetMemoryDepth (void) const
{
	return ((TParamSet *) m_Table[m_nSelectedIndex])->nMemoryDepth;
}

unsigned CScopeConfig::GetChartScale (void) const
{
	return ((TParamSet *) m_Table[m_nSelectedIndex])->nChartScale;
}

unsigned CScopeConfig::GetActualRate (unsigned nIndex) const
{
	assert (nIndex < m_Table.GetCount ());
	return ((TParamSet *) m_Table[nIndex])->nActualRateKHz;
}
