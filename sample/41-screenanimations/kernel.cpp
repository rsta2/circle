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

CKernel::CKernel (void)
:
#ifdef SPI_DISPLAY
	m_SPIMaster (SPI_CLOCK_SPEED, SPI_CPOL, SPI_CPHA, SPI_MASTER_DEVICE),
	m_SPIDisplay (&m_SPIMaster, DISPLAY_PARAMETERS),
	m_2DGraphics (&m_SPIDisplay)
#elif defined (I2C_DISPLAY)
	m_I2CMaster (I2C_MASTER_DEVICE, TRUE),			// TRUE: I2C fast mode
	m_I2CDisplay (&m_I2CMaster, DISPLAY_PARAMETERS),
	m_2DGraphics (&m_I2CDisplay)
#else
	m_2DGraphics (m_Options.GetWidth (), m_Options.GetHeight (), TRUE)
#endif
{
	m_ActLED.Blink (5);
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
#ifdef SPI_DISPLAY
	if (!m_SPIMaster.Initialize ())
	{
		return FALSE;
	}

#ifdef DISPLAY_ROTATION
	m_SPIDisplay.SetRotation (DISPLAY_ROTATION);
#endif

	if (!m_SPIDisplay.Initialize ())
	{
		return FALSE;
	}

#elif defined (I2C_DISPLAY)
	if (!m_I2CMaster.Initialize ())
	{
		return FALSE;
	}

#ifdef DISPLAY_ROTATION
	m_I2CDisplay.SetRotation (DISPLAY_ROTATION);
#endif

	if (!m_I2CDisplay.Initialize ())
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
