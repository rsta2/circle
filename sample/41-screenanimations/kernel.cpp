//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2024  R. Stange <rsta2@o2online.de>
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

#ifdef USE_ST7789
	#include "st7789config.h"
#endif

CKernel::CKernel (void)
:
#ifndef USE_ST7789
	m_2DGraphics (m_Options.GetWidth (), m_Options.GetHeight (), TRUE)
#else
	m_SPIMaster (SPI_CLOCK_SPEED, SPI_CPOL, SPI_CPHA, SPI_MASTER_DEVICE),
	m_ST7789 (&m_SPIMaster, DC_PIN, RESET_PIN, BACKLIGHT_PIN, WIDTH, HEIGHT,
		  SPI_CPOL, SPI_CPHA, SPI_CLOCK_SPEED, SPI_CHIP_SELECT),
	m_2DGraphics (&m_ST7789)
#endif
{
	m_ActLED.Blink (5);
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
#ifdef USE_ST7789
	if (!m_SPIMaster.Initialize ())
	{
		return FALSE;
	}

	if (!m_ST7789.Initialize ())
	{
		return FALSE;
	}
#endif

	return m_2DGraphics.Initialize ();
}

TShutdownMode CKernel::Run (void)
{
	for (unsigned i = 0; i < nShapes; i++)
	{
		m_pShape[i] = new CGraphicShape (&m_2DGraphics);
	}

	while (1)
	{
		m_2DGraphics.ClearScreen(CDisplay::Black);
		
		for(unsigned i=0; i<nShapes; i++)
		{
			m_pShape[i]->Draw();
		}
		
		m_2DGraphics.UpdateDisplay();
	}

	return ShutdownHalt;
}
