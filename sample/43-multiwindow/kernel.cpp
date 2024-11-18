//
// kernel.cpp
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
#include "kernel.h"
#include <circle/screen.h>		// for DEPTH
#include <lvgl/lvgl/demos/lv_demos.h>
#include <assert.h>

LOGMODULE ("kernel");

CKernel::CKernel (void)
:	m_FrameBuffer (m_Options.GetWidth (), m_Options.GetHeight (), DEPTH),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer, TRUE)
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
		bOK = m_FrameBuffer.Initialize ();
	}

	for (unsigned i = 0; i < NUM_WINDOWS; i++)
	{
		unsigned nHeight = m_FrameBuffer.GetHeight () / 2;
		unsigned nWidth = m_FrameBuffer.GetWidth () / 2;

		CDisplay::TArea Area;
		Area.x1 = nWidth * (i % 2);
		Area.x2 = Area.x1 + nWidth - 1;
		Area.y1 = nHeight * ((i >> 1) % 2);
		Area.y2 = Area.y1 + nHeight - 1;

		m_pWindow[i] = new CWindowDisplay (&m_FrameBuffer, Area);
		assert (m_pWindow[i]);
	}

	if (bOK)
	{
		m_pTerminal = new CTerminalDevice (m_pWindow[TERMINAL_WINDOW]);
		assert (m_pTerminal);

		bOK = m_pTerminal->Initialize ();
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
			pTarget = &m_pTerminal[TERMINAL_WINDOW];
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

	if (bOK)
	{
		m_pDemo = new CMultiCoreDemo (m_pWindow);
		assert (m_pDemo);

		bOK = m_pDemo->Initialize ();
	}

	if (bOK)
	{
#if RASPPI <= 4
		m_Touchscreen.Initialize ();
#endif

		assert (m_pWindow[LVGL_WINDOW]);
		m_pGUI = new CLVGL (m_pWindow[LVGL_WINDOW]);
		assert (m_pGUI);

		bOK = m_pGUI->Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	LOGNOTE ("Compile time: " __DATE__ " " __TIME__);

	lv_demo_widgets ();

	while (1)
	{
		boolean bUpdated = m_USBHCI.UpdatePlugAndPlay ();

		assert (m_pGUI);
		m_pGUI->Update (bUpdated);
	}

	return ShutdownHalt;
}
