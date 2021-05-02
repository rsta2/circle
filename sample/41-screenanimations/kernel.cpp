//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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

CKernel::CKernel (void)
: m_2DGraphics(m_Options.GetWidth (), m_Options.GetHeight ())
{
	m_ActLED.Blink (5);
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	return m_2DGraphics.Initialize ();
}

TShutdownMode CKernel::Run (void)
{
	for (unsigned i = 0; i < nShapes; i++)
	{
		m_pShape[i] = new CGraphicShape (m_2DGraphics.GetWidth (), m_2DGraphics.GetHeight ());
	}

	while (1)
	{
		m_2DGraphics.ClearScreen(BLACK_COLOR);
		
		for(unsigned i=0; i<nShapes; i++)
		{
			m_pShape[i]->Draw(&m_2DGraphics);
		}
		
		m_2DGraphics.UpdateDisplay();
	}

	return ShutdownHalt;
}
