//
// recorder.cpp
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
#include "recorder.h"
#include "sampler.h"
#include <circle/machineinfo.h>
#include <circle/logger.h>
#include <circle/synchronize.h>
#include <assert.h>

static const char FromRecorder[] = "recorder";

// must have increasing order
unsigned CRecorder::s_nWantedRateKHz[] = {100, 1000, 2000, 5000, 10000, 15000};

CRecorder::CRecorder (unsigned nCh1Pin, unsigned nCh2Pin, unsigned nCh3Pin, unsigned nCh4Pin,
		      CScopeConfig *pConfig)
:	m_nCh1Pin (nCh1Pin),
	m_nCh2Pin (nCh2Pin),
	m_nCh3Pin (nCh3Pin),
	m_nCh4Pin (nCh4Pin),
	m_pConfig (pConfig),
	m_Ch1 (nCh1Pin, GPIOModeInput),
	m_Ch2 (nCh2Pin, GPIOModeInput),
	m_Ch3 (nCh3Pin, GPIOModeInput),
	m_Ch4 (nCh4Pin, GPIOModeInput),
	m_pSamples (0),
	m_nMemoryDepth (MAX_MEMORY_DEPTH),
	m_nTriggerMask (0),
	m_nTriggerLevel (0),
	m_nDelayCount (0),
	m_nRuntime (0)
{
	CMachineInfo MachineInfo;
	m_nCPUClockKHz = MachineInfo.GetClockRate (CLOCK_ID_ARM) / 1000;
}

CRecorder::~CRecorder (void)
{
	delete [] m_pSamples;
	m_pSamples = 0;
}

boolean CRecorder::Initialize (void)
{
	assert (m_pSamples == 0);
	m_pSamples = new u32[MAX_MEMORY_DEPTH];
	if (m_pSamples == 0)
	{
		CLogger::Get ()->Write (FromRecorder, LogError, "Cannot allocate buffer");

		return FALSE;
	}

	CLogger::Get ()->Write (FromRecorder, LogDebug, "Calibrating sampler:");

	for (unsigned i = 0; i < sizeof s_nWantedRateKHz / sizeof s_nWantedRateKHz[0]; i++)
	{
		unsigned nActualRateKHz;
		unsigned nDelayCount = Calibrate (s_nWantedRateKHz[i], &nActualRateKHz);

		assert (m_pConfig != 0);
		m_pConfig->AddParamSet (s_nWantedRateKHz[i], nActualRateKHz, nDelayCount);

		CLogger::Get ()->Write (FromRecorder, LogDebug, "Target %uKHz / Actual %uKHz / Count %u",
					s_nWantedRateKHz[i], nActualRateKHz, nDelayCount);

		if (nDelayCount == 0)
		{
			break;
		}

		if (100*nActualRateKHz / s_nWantedRateKHz[i] < 85)
		{
			break;
		}
	}

	return TRUE;
}

unsigned CRecorder::GetPin (unsigned nChannel) const
{
	assert (nChannel > 0);
	assert (nChannel <= CHANS);

	switch (nChannel)
	{
	case 1:		return m_nCh1Pin;
	case 2:		return m_nCh2Pin;
	case 3:		return m_nCh3Pin;
	case 4:		return m_nCh4Pin;

	default:	return GPIO_PINS;
	}
}

void CRecorder::SetMemoryDepth (unsigned nMemoryDepth)
{
	assert (nMemoryDepth <= MAX_MEMORY_DEPTH);
	m_nMemoryDepth = nMemoryDepth;
}

void CRecorder::SetTrigger (u32 nMask, u32 nLevel)
{
	m_nTriggerMask  = 0;
	m_nTriggerLevel = 0;

	for (unsigned i = 1; i <= CHANS; i++)
	{
		if (nMask & CH(i))
		{
			m_nTriggerMask |= 1 << GetPin (i);
		}

		if (nLevel & CH(i))
		{
			m_nTriggerLevel |= 1 << GetPin (i);
		}
	}
}

void CRecorder::SetDelayCount (u32 nDelayCount)
{
	m_nDelayCount = nDelayCount;
}

boolean CRecorder::Run (void)
{
	assert (m_pSamples != 0);
	assert (m_nMemoryDepth > 0);
	assert ((m_nTriggerLevel & ~m_nTriggerMask) == 0);

	CleanDataCache ();
	InvalidateDataCache ();
	m_nRuntime = Sampler (m_pSamples, m_nMemoryDepth, m_nTriggerMask, m_nTriggerLevel, m_nDelayCount);

	if (m_nRuntime == 0)
	{
		return FALSE;
	}

	return TRUE;
}

unsigned CRecorder::GetRuntime (void) const
{
	return m_nRuntime;
}

unsigned CRecorder::GetSample (unsigned nChannel, unsigned nOffset) const
{
	u32 nMask = 1 << GetPin (nChannel);

	assert (nOffset < m_nMemoryDepth);
	assert (m_pSamples != 0);
	return m_pSamples[nOffset] & nMask ? HIGH : LOW;
}

unsigned CRecorder::Calibrate (unsigned nWantedRateKHz, unsigned *pActualRateKHz)
{
	// Binary search for the optimum delay count to reach nWantedRateKHz

	unsigned nWantedRuntime = 1000 * CALIBRATE_DEPTH / nWantedRateKHz;

	unsigned nLowerBound = 0;
	unsigned nUpperBound = m_nCPUClockKHz / nWantedRateKHz;

	unsigned nDelayCount;
	unsigned nRuntime = 0;

	while (1)
	{
		nDelayCount = nLowerBound + (nUpperBound-nLowerBound+1) / 2;

		if (nUpperBound == nLowerBound)
		{
			break;
		}

		nRuntime = RunCalibrate (nDelayCount);
		assert (nRuntime != 0);

		if (nRuntime == nWantedRuntime)
		{
			break;
		}
		else if (nRuntime > nWantedRuntime)
		{
			nUpperBound = nLowerBound + (nUpperBound-nLowerBound) / 2;
		}
		else
		{
			nLowerBound = nDelayCount;
		}
	}

	nDelayCount = CalibrateFine (nDelayCount, nWantedRuntime);

	nRuntime = RunCalibrate (nDelayCount);

	assert (pActualRateKHz != 0);
	assert (nRuntime != 0);
	*pActualRateKHz = (1000*CALIBRATE_DEPTH + nRuntime/2) / nRuntime;

	return nDelayCount;
}

unsigned CRecorder::CalibrateFine (unsigned nDelayCount, unsigned nWantedRuntime)
{
	// Find minimum error for nDelayCount / nDelayCount+1 / nDelayCount-1

	int nError0 = (int) RunCalibrate (nDelayCount) - (int) nWantedRuntime;
	if (nError0 < 0)
	{
		nError0 = -nError0;
	}

	int nError1 = (int) RunCalibrate (nDelayCount+1) - (int) nWantedRuntime;
	if (nError1 < 0)
	{
		nError1 = -nError1;
	}

	int nError2 = 1000000;
	if (nDelayCount > 0)
	{
		int nError2 = (int) RunCalibrate (nDelayCount-1) - (int) nWantedRuntime;
		if (nError2 < 0)
		{
			nError2 = -nError2;
		}
	}

	if (nError1 < nError0)
	{
		if (nError2 < nError1)
		{
			nDelayCount--;
		}
		else
		{
			nDelayCount++;
		}
	}
	else
	{
		if (nError2 < nError0)
		{
			nDelayCount--;
		}
	}

	return nDelayCount;
}

unsigned CRecorder::RunCalibrate (unsigned nDelayCount)
{
	assert (m_pSamples != 0);

	CleanDataCache ();
	InvalidateDataCache ();
	return Sampler (m_pSamples, CALIBRATE_DEPTH, 0, 0, nDelayCount);
}
