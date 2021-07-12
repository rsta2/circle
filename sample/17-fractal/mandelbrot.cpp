//
// mandelbrot.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2021  R. Stange <rsta2@o2online.de>
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

#define MAX_ITERATION	5000

CMandelbrotCalculator::CMandelbrotCalculator (CScreenDevice *pScreen, CMemorySystem *pMemorySystem)
:	
#ifdef ARM_ALLOW_MULTI_CORE
	CMultiCoreSupport (pMemorySystem),
#endif
	m_pScreen (pScreen)
{
}

CMandelbrotCalculator::~CMandelbrotCalculator (void)
{
	m_pScreen = 0;
}

void CMandelbrotCalculator::Run (unsigned nCore)
{
#ifdef ARM_ALLOW_MULTI_CORE
	unsigned nQuarterHeight = m_pScreen->GetHeight () / 4;

	switch (nCore)
	{
	case 0:
		Calculate (-2.0, 1.0, -1.0, -0.5, MAX_ITERATION, 0, nQuarterHeight);
		break;

	case 1:
		Calculate (-2.0, 1.0, -0.5, 0.0, MAX_ITERATION, nQuarterHeight, nQuarterHeight);
		break;

	case 2:
		Calculate (-2.0, 1.0, 0.0, 0.5, MAX_ITERATION, nQuarterHeight*2, nQuarterHeight);
		break;

	case 3:
		Calculate (-2.0, 1.0, 0.5, 1.0, MAX_ITERATION, nQuarterHeight*3, nQuarterHeight);
		break;
	}
#else
	Calculate (-2.0, 1.0, -1.0, 1.0, MAX_ITERATION, 0, m_pScreen->GetHeight ());
#endif
}

// See: http://en.wikipedia.org/wiki/Mandelbrot_set
void CMandelbrotCalculator::Calculate (float x1, float x2, float y1, float y2, unsigned nMaxIteration,
				       unsigned nPosY0, unsigned nHeight)
{
	float dx = (x2-x1) / m_pScreen->GetWidth ();
	float dy = (y2-y1) / nHeight;

	float y0 = y1;
	for (unsigned nPosY = nPosY0; nPosY < nPosY0+nHeight; nPosY++, y0 += dy)
	{
		float x0 = x1;
		for (unsigned nPosX = 0; nPosX < m_pScreen->GetWidth (); nPosX++, x0 += dx)
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
			TScreenColor Color = (TScreenColor) (nIteration * 15 / nMaxIteration);
#elif DEPTH == 16
			TScreenColor Color = (TScreenColor) (nIteration * 65535 / nMaxIteration);
			Color++;
#else
	#error DEPTH must be 8 or 16
#endif
			m_pScreen->SetPixel (nPosX, nPosY, Color);
		}
	}
}
