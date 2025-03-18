//
// multicoredemo.cpp
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
#include "multicoredemo.h"
#include "graphicshape.h"
#include "mandelbrot.h"
#include <circle/2dgraphics.h>
#include <circle/device.h>
#include <circle/devicenameservice.h>
#include <circle/timer.h>
#include <circle/memory.h>
#include <circle/string.h>
#include <assert.h>

CMultiCoreDemo::CMultiCoreDemo (CDisplay **ppWindow)
:	CMultiCoreSupport (CMemorySystem::Get ()),
	m_ppWindow (ppWindow)
{
	assert (m_ppWindow);
}

CMultiCoreDemo::~CMultiCoreDemo (void)
{
}

void CMultiCoreDemo::Run (unsigned nCore)
{
	switch (nCore)
	{
	case 1:
		Graphics2DDemo ();
		break;

	case 2:
		MandelbrotDemo ();
		break;

	case 3:
		TerminalDemo ();
		break;
	}
}

void CMultiCoreDemo::Graphics2DDemo (void)
{
	assert (m_ppWindow[GRAPHICS2D_WINDOW]);
	C2DGraphics Graphics2D (m_ppWindow[GRAPHICS2D_WINDOW]);
	if (!Graphics2D.Initialize ())
	{
		return;
	}

	const unsigned Shapes = 32;
	CGraphicShape *m_pShape[Shapes];
	for (unsigned i = 0; i < Shapes; i++)
	{
		m_pShape[i] = new CGraphicShape (&Graphics2D);
		assert (m_pShape[i]);
	}

	while (1)
	{
		Graphics2D.ClearScreen (CDisplay::Black);

		for (unsigned i = 0; i < Shapes; i++)
		{
			m_pShape[i]->Draw ();
		}

		Graphics2D.UpdateDisplay ();
	}
}

void CMultiCoreDemo::MandelbrotDemo (void)
{
	assert (m_ppWindow[MANDELBROT_WINDOW]);
	CMandelbrotCalculator Mandelbrot (m_ppWindow[MANDELBROT_WINDOW]);

	while (1)
	{
		Mandelbrot.Run ();

		CTimer::Get ()->MsDelay (5000);
	}
}

void CMultiCoreDemo::TerminalDemo (void)
{
	CDevice *pTerminal = CDeviceNameService::Get ()->GetDevice ("tty1", FALSE);
	if (!pTerminal)
	{
		return;
	}

	for (unsigned i = 1; 1; i++)
	{
		CTimer::Get ()->MsDelay (5000);

		CString String;
		String.Format ("Hello terminal (%u) ...\n", i);

		pTerminal->Write (String.c_str (), String.GetLength ());
	}
}
