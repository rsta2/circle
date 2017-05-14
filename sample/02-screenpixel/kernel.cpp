//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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
:	m_Memory (FALSE),	// set this to TRUE to enable MMU and to boost performance
	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ())
{
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	return m_Screen.Initialize ();
}

TShutdownMode CKernel::Run (void)
{
	// draw rectangle on screen
	for (unsigned nPosX = 0; nPosX < m_Screen.GetWidth (); nPosX++)
	{
		m_Screen.SetPixel (nPosX, 0, NORMAL_COLOR);
		m_Screen.SetPixel (nPosX, m_Screen.GetHeight ()-1, NORMAL_COLOR);
	}
	for (unsigned nPosY = 0; nPosY < m_Screen.GetHeight (); nPosY++)
	{
		m_Screen.SetPixel (0, nPosY, NORMAL_COLOR);
		m_Screen.SetPixel (m_Screen.GetWidth ()-1, nPosY, NORMAL_COLOR);
	}

	// draw cross on screen
	for (unsigned nPosX = 0; nPosX < m_Screen.GetWidth (); nPosX++)
	{
		unsigned nPosY = nPosX * m_Screen.GetHeight () / m_Screen.GetWidth ();

		m_Screen.SetPixel (nPosX, nPosY, NORMAL_COLOR);
		m_Screen.SetPixel (m_Screen.GetWidth ()-nPosX-1, nPosY, NORMAL_COLOR);
	}

	// check the blink frequency without and with MMU (see option in constructor above)
	while (1)
	{
		m_ActLED.On ();
		for (volatile unsigned i = 1; i <= 5000000; i++)
		{
			// just wait
		}

		m_ActLED.Off ();
		for (volatile unsigned i = 1; i <= 10000000; i++)
		{
			// just wait
		}
	}

	return ShutdownHalt;
}
