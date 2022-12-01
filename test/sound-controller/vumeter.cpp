//
// vumeter.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2022  R. Stange <rsta2@o2online.de>
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
#include "vumeter.h"
#include "config.h"

// level decreases from 1 to MinValue in DecaySeconds time, when input is 0
static const float DecaySeconds	= 2.0f;
static const float MinValue	= 0.001f;
static const float LogMinValue	= -6.907755f;	// log(MinValue)

CVUMeter::CVUMeter (CScreenDevice *pScreen, unsigned nPosX, unsigned nPosY,
		    unsigned nWidth, unsigned nHeight)
:	m_pScreen (pScreen),
	m_nPosX (nPosX),
	m_nPosY (nPosY),
	m_nWidth (nWidth),
	m_nHeight (nHeight),
	m_fFactor (exp (LogMinValue / DecaySeconds / SAMPLE_RATE)),
	m_fValue (0.0f)
{
}

CVUMeter::~CVUMeter (void)
{
	assert (m_pScreen);

	for (unsigned x = 0; x < m_nWidth; x++)
	{
		for (unsigned y = 0; y < m_nHeight; y++)
		{
			m_pScreen->SetPixel (m_nPosX + x, m_nPosY + y, BLACK_COLOR);
		}
	}
}

void CVUMeter::PutInputLevel (float fLevel)
{
	if (fLevel < 0.0f)
	{
		fLevel = -fLevel;
	}

	if (fLevel > m_fValue)
	{
		m_fValue = fLevel;
	}
	else if (m_fValue > MinValue)
	{
		m_fValue *= m_fFactor;
	}
	else
	{
		m_fValue = 0.0f;
	}
}

void CVUMeter::Update (void)
{
	assert (m_pScreen);

	const unsigned nValue  = m_nWidth * m_fValue;
	const unsigned nYellow = m_nWidth * 0.8f;
	const unsigned nRed    = m_nWidth * 0.9f;

	for (unsigned x = 0; x < m_nWidth; x++)
	{
		TScreenColor Color = WHITE_COLOR;
		if (x <= nValue)
		{
			if (x >= nRed)
			{
				Color = RED_COLOR;
			}
			else
			{
				if (x >= nYellow)
				{
					Color = BRIGHT_YELLOW_COLOR;
				}
				else
				{
					Color = GREEN_COLOR;
				}
			}
		}

		for (unsigned y = 0; y < m_nHeight; y++)
		{
			m_pScreen->SetPixel (m_nPosX + x, m_nPosY + y, Color);
		}
	}
}

float CVUMeter::exp (float x)
{
	int s = 0;
	if (x < 0)
	{
		s = 1;
		x = -x;
	}

	unsigned n = 0;
	while (x > 0.01f)
	{
		x /= 2;
		n++;
	}

	float t = x+1;
	float y = (t*t + 1) / 2;

	while (n--)
	{
		y *= y;
	}

	if (s)
	{
		y = 1 / y;
	}

	return y;
}
