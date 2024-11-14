//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019-2024  R. Stange <rsta2@o2online.de>
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
#include "../lvgl/demos/lv_demos.h"

#ifdef USE_ST7789
	#include "st7789config.h"
#endif

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer, TRUE),
#ifndef USE_ST7789
	m_GUI (&m_Screen)
#else
	m_SPIMaster (SPI_CLOCK_SPEED, SPI_CPOL, SPI_CPHA, SPI_MASTER_DEVICE),
	m_ST7789 (&m_SPIMaster, DC_PIN, RESET_PIN, BACKLIGHT_PIN, WIDTH, HEIGHT,
		  SPI_CPOL, SPI_CPHA, SPI_CLOCK_SPEED, SPI_CHIP_SELECT, FALSE),
	m_GUI (&m_ST7789)
#endif
{
	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Serial.Initialize (115200);
	}

	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Screen;
		}

		bOK = m_Logger.Initialize (pTarget);
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	if (bOK)
	{
		bOK = m_USBHCI.Initialize ();
	}

#ifdef USE_ST7789
	if (bOK)
	{
		bOK = m_SPIMaster.Initialize ();
	}

	if (bOK)
	{
		bOK = m_ST7789.Initialize ();
	}
#else
	if (bOK)
	{
		m_RPiTouchScreen.Initialize ();
	}
#endif

	if (bOK)
	{
		bOK = m_GUI.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	lv_demo_widgets ();

	while (1)
	{
		boolean bUpdated = m_USBHCI.UpdatePlugAndPlay ();

		m_GUI.Update (bUpdated);
	}

	return ShutdownHalt;
}
