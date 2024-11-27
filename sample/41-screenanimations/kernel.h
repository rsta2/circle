//
// kernel.h
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
#ifndef _kernel_h
#define _kernel_h

#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/2dgraphics.h>
#include <circle/types.h>
#include "graphicshape.h"

#ifdef SPI_DISPLAY
	#include <circle/spimaster.h>
	#include <display/sampleconfig.h>
#elif defined (I2C_DISPLAY)
	#include <circle/i2cmaster.h>
	#include <display/sampleconfig.h>
#endif

enum TShutdownMode
{
	ShutdownNone,
	ShutdownHalt,
	ShutdownReboot
};

class CKernel
{
private:
	static const unsigned nShapes = 32;

public:
	CKernel (void);
	~CKernel (void);

	boolean Initialize (void);

	TShutdownMode Run (void);

private:
	// do not change this order
	CActLED			m_ActLED;
	CKernelOptions		m_Options;
#ifdef SPI_DISPLAY
	CSPIMaster		m_SPIMaster;
	DISPLAY_CLASS		m_SPIDisplay;
#elif defined (I2C_DISPLAY)
	CI2CMaster		m_I2CMaster;
	DISPLAY_CLASS		m_I2CDisplay;
#endif
	C2DGraphics		m_2DGraphics;
	CGraphicShape		*m_pShape[nShapes];
};

#endif
