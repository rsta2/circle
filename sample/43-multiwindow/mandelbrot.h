//
// mandelbrot.h
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
#ifndef _mandelbrot_h
#define _mandelbrot_h

#include <circle/display.h>

class CMandelbrotCalculator
{
public:
	CMandelbrotCalculator (CDisplay *pDisplay);
	~CMandelbrotCalculator (void);

	void Run (void);

private:
	void Calculate (float x1, float x2, float y1, float y2, unsigned nMaxIteration,
			unsigned nPosY0, unsigned nHeight);

private:
	CDisplay *m_pDisplay;
};

#endif
