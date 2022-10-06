//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2022  R. Stange <rsta2@o2online.de>
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
#include "kernel.h"
#include <assert.h>
#include <circle/synchronize.h>
#include <circle/types.h>
#include "sampler.h"

#define GPIO_INPUT_PIN		18
#define GPIO_INPUT_PIN_MASK	(1 << GPIO_INPUT_PIN)

#define CLOCK_RATE		125000

#define SAMPLING_DELAY		0		// controls sampling rate, 0 is fastest

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_Clock0 (GPIOClock0),
	m_ClockPin (4, GPIOModeAlternateFunction0),
	m_InputPin (GPIO_INPUT_PIN, GPIOModeInput)
{
	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Serial.Initialize (115200);
	}

	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Screen;
		}

		bOK = m_Logger.Initialize (pTarget);
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	if (!m_Clock0.StartRate (CLOCK_RATE))
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot generate %u Hz clock", CLOCK_RATE);
	}

	size_t nSamples = m_Screen.GetWidth ();
	assert (nSamples > 0);
	
	u32 *pBuffer = new u32[nSamples];
	if (pBuffer == 0)
	{
		m_Logger.Write (FromKernel, LogPanic, "Not enough memory");
	}

	CleanDataCache ();
	InvalidateDataCache ();

	u32 nRunTime = Sampler (pBuffer, nSamples, 0, 0, SAMPLING_DELAY);
	if (nRunTime == 0)
	{
		m_Logger.Write (FromKernel, LogPanic, "Measurement of run time failed, try again!");
	}

	unsigned nSampleRateKHz = 1000 * nSamples / nRunTime;
	m_Logger.Write (FromKernel, LogNotice, "Sampling rate was %u KHz", nSampleRateKHz);
	m_Logger.Write (FromKernel, LogNotice, "Display covers %u us", nRunTime);
	m_Logger.Write (FromKernel, LogNotice, "A red line is drawn on every 10 us");

	unsigned nPosYLow  = m_Screen.GetHeight () / 2 + 40;
	unsigned nPosYHigh = m_Screen.GetHeight () / 2 - 40;

	boolean bLastLevel = FALSE;
	unsigned nLastMarker = 0;
	for (unsigned nSample = 0; nSample < nSamples; nSample++)
	{
		unsigned nTime = nRunTime * nSample / nSamples / 10;
		if (nLastMarker != nTime)
		{
			for (unsigned nPosY = nPosYHigh-20; nPosY <= nPosYLow+20; nPosY++)
			{
				m_Screen.SetPixel (nSample, nPosY, HIGH_COLOR);
			}

			nLastMarker = nTime;
		}

		assert (pBuffer != 0);
		boolean bThisLevel = pBuffer[nSample] & GPIO_INPUT_PIN_MASK ? TRUE : FALSE;

		unsigned nPosY = bThisLevel ? nPosYHigh : nPosYLow;
		m_Screen.SetPixel (nSample, nPosY,   NORMAL_COLOR);
		m_Screen.SetPixel (nSample, nPosY+1, NORMAL_COLOR);
		m_Screen.SetPixel (nSample, nPosY+2, NORMAL_COLOR);

		if (   nSample > 0
		    && bLastLevel != bThisLevel)
		{
			for (unsigned nPosY = nPosYHigh; nPosY <= nPosYLow+2; nPosY++)
			{
				m_Screen.SetPixel (nSample, nPosY, NORMAL_COLOR);
			}
		}

		bLastLevel = bThisLevel;
	}

	delete [] pBuffer;
	
	return ShutdownHalt;
}
