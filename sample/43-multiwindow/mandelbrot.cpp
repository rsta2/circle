//
// mandelbrot.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2024  R. Stange <rsta2@o2online.de>
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
#include "mandelbrot.h"
#include <circle/screen.h>

#define MAX_ITERATION	5000

CMandelbrotCalculator::CMandelbrotCalculator (CDisplay *pDisplay)
:	m_pDisplay (pDisplay)
{
}

CMandelbrotCalculator::~CMandelbrotCalculator (void)
{
}

void CMandelbrotCalculator::Run (void)
{
	// Clear window
	for (unsigned y = 0; y < m_pDisplay->GetHeight (); y++)
	{
		for (unsigned x = 0; x < m_pDisplay->GetWidth (); x++)
		{
			m_pDisplay->SetPixel (x, y, 0);
		}
	}

	Calculate (-2.0, 1.0, -1.0, 1.0, MAX_ITERATION, 0, m_pDisplay->GetHeight ());
}

// See: http://en.wikipedia.org/wiki/Mandelbrot_set
void CMandelbrotCalculator::Calculate (float x1, float x2, float y1, float y2,
				       unsigned nMaxIteration, unsigned nPosY0, unsigned nHeight)
{
	float dx = (x2-x1) / m_pDisplay->GetWidth ();
	float dy = (y2-y1) / nHeight;

	float y0 = y1;
	for (unsigned nPosY = nPosY0; nPosY < nPosY0+nHeight; nPosY++, y0 += dy)
	{
		float x0 = x1;
		for (unsigned nPosX = 0; nPosX < m_pDisplay->GetWidth (); nPosX++, x0 += dx)
		{
			float x = 0.0;
			float y = 0.0;
			unsigned nIteration = 0;
			for (; x*x+y*y < 2*2 && nIteration < nMaxIteration; nIteration++)
			{
				float xtmp = x*x - y*y + x0;
				y = 2*x*y + y0;
				x = xtmp;
			}

#if DEPTH == 8
			CDisplay::TRawColor Color = nIteration * 15 / nMaxIteration;
#elif DEPTH == 16
			CDisplay::TRawColor Color = nIteration * 65535 / nMaxIteration;
			Color++;
#else
	#error DEPTH must be 8 or 16
#endif
			m_pDisplay->SetPixel (nPosX, nPosY, Color);
		}
	}
}
