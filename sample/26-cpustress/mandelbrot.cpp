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
#include <circle/sched/scheduler.h>

#define LINES_LEFT_FREE	100		// at the bottom of the screen

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
	while (1)
	{
#ifdef ARM_ALLOW_MULTI_CORE
		unsigned nHalfWidth  = m_pScreen->GetWidth () / 2;
		unsigned nHalfHeight = (m_pScreen->GetHeight ()-LINES_LEFT_FREE) / 2;

		switch (nCore)
		{
		case 0:
			Calculate (-2.0, 1.0, -1.0, 1.0, MAX_ITERATION,
				   0, nHalfWidth, 0, nHalfHeight);
			break;

		case 1:
			Calculate (-1.5, 0.0, -0.75, 0.25, MAX_ITERATION,
				   nHalfWidth, nHalfWidth, 0, nHalfHeight);
			break;

		case 2:
			Calculate (-1.125, -0.375, -0.5, 0.0, MAX_ITERATION,
				   0, nHalfWidth, nHalfHeight, nHalfHeight);
			break;

		case 3:
			Calculate (-0.9375, -0.5625, -0.375, -0.125, MAX_ITERATION,
				   nHalfWidth, nHalfWidth, nHalfHeight, nHalfHeight);
			break;
		}
#else
		Calculate (-2.0, 1.0, -1.0, 1.0, MAX_ITERATION,
			   0, m_pScreen->GetWidth (), 0, m_pScreen->GetHeight ()-LINES_LEFT_FREE);
#endif
	}
}

// See: http://en.wikipedia.org/wiki/Mandelbrot_set
void CMandelbrotCalculator::Calculate (double x1, double x2, double y1, double y2, unsigned nMaxIteration,
				       unsigned nPosX0, unsigned nWidth, unsigned nPosY0, unsigned nHeight)
{
	// clear image
	for (unsigned nPosY = nPosY0; nPosY < nPosY0+nHeight; nPosY++)
	{
		for (unsigned nPosX = nPosX0; nPosX < nPosX0+nWidth; nPosX++)
		{
			m_pScreen->SetPixel (nPosX, nPosY, BLACK_COLOR);
		}
	}

	double dx = (x2-x1) / nWidth;
	double dy = (y2-y1) / nHeight;

	double y0 = y1;
	for (unsigned nPosY = nPosY0; nPosY < nPosY0+nHeight; nPosY++, y0 += dy)
	{
		double x0 = x1;
		for (unsigned nPosX = nPosX0; nPosX < nPosX0+nWidth; nPosX++, x0 += dx)
		{
			double x = 0.0;
			double y = 0.0;
			unsigned nIteration = 0;
			for (; x*x+y*y < 2*2 && nIteration < nMaxIteration; nIteration++)
			{
				double xtmp = x*x - y*y + x0;
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

		// let the temperature task run
#ifdef ARM_ALLOW_MULTI_CORE
		if (ThisCore () == 0)
		{
			CScheduler::Get ()->Yield ();
		}
#else
		CScheduler::Get ()->Yield ();
#endif
	}
}
