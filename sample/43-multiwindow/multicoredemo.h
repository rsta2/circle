//
// multicoredemo.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2024  R. Stange <rsta2@o2online.de>
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
#ifndef _multicoredemo_h
#define _multicoredemo_h

#include <circle/multicore.h>
#include <circle/display.h>

// Window assignment
#define LVGL_WINDOW		0
#define GRAPHICS2D_WINDOW	1
#define MANDELBROT_WINDOW	2
#define TERMINAL_WINDOW		3

#define NUM_WINDOWS		4

class CMultiCoreDemo : public CMultiCoreSupport
{
public:
	CMultiCoreDemo (CDisplay **ppWindow);
	~CMultiCoreDemo (void);

	void Run (unsigned nCore);

private:
	void Graphics2DDemo (void);
	void MandelbrotDemo (void);
	void TerminalDemo (void);

public:
	CDisplay **m_ppWindow;
};

#endif
