//
// temptask.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2022  R. Stange <rsta2@o2online.de>
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
#include "temptask.h"
#include <circle/sched/scheduler.h>
#include <circle/cputhrottle.h>
#include <circle/logger.h>
#include <assert.h>

#define MAX_GOOD_TEMP		79	// recommended maximum
#define MAX_ALLOWED_TEMP	84
#define MAX_DISPLAY_TEMP	99

static const char FromTempTask[] = "temptask";

CTemperatureTask::CTemperatureTask (CScreenDevice  *pScreen)
:	m_pScreen (pScreen)
{
}

CTemperatureTask::~CTemperatureTask (void)
{
}

void CTemperatureTask::Run (void)
{
	assert (m_pScreen != 0);

	for (unsigned nPosX = 0; 1; nPosX++)
	{
		if (nPosX >= m_pScreen->GetWidth ())
		{
			nPosX = 0;
		}

		unsigned nCelsius = CCPUThrottle::Get ()->GetTemperature ();
		if (nCelsius == 0)
		{
			CCPUThrottle::Get ()->SetSpeed (CPUSpeedLow);

			CLogger::Get ()->Write (FromTempTask, LogPanic, "Cannot get SoC temperature");
		}

		if (nCelsius > MAX_DISPLAY_TEMP)
		{
			nCelsius = MAX_DISPLAY_TEMP;
		}

		// draw point and fill area
		for (unsigned i = 0; i <= nCelsius; i++)
		{
			unsigned nPosY = m_pScreen->GetHeight ()-1 - i;

			TScreenColor Color;
			if (nCelsius <= MAX_GOOD_TEMP)
			{
				Color = HALF_COLOR;
			}
			else if (nCelsius <= MAX_ALLOWED_TEMP)
			{
				Color = NORMAL_COLOR;
			}
			else
			{
				Color = HIGH_COLOR;
			}

			m_pScreen->SetPixel (nPosX, nPosY, Color);
		}

		// draw a white line every 20 degrees Celsius
		for (unsigned i = 0; i <= MAX_DISPLAY_TEMP; i += 20)
		{
			unsigned nPosY = m_pScreen->GetHeight ()-1 - i;

			m_pScreen->SetPixel (nPosX, nPosY, NORMAL_COLOR);
		}

		// draw a red line for the maximum allowed temperature + 1
		m_pScreen->SetPixel (nPosX, m_pScreen->GetHeight ()-1 - (MAX_ALLOWED_TEMP+1), HIGH_COLOR);

		if (!CCPUThrottle::Get ()->SetOnTemperature ())
		{
			CCPUThrottle::Get ()->SetSpeed (CPUSpeedLow);

			CLogger::Get ()->Write (FromTempTask, LogPanic, "CPU temperature management failed");
		}

		CScheduler::Get ()->Sleep (2);
	}
}
