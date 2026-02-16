//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2025  R. Stange <rsta2@o2online.de>
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
#include <circle/string.h>
#include <circle/util.h>
#include <circle/machineinfo.h>

static const char FromKernel[] = "kernel";

#define I2C_MASTER_DEVICE	(CMachineInfo::Get ()->GetDevice (DeviceI2CMaster))

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_I2CMaster (I2C_MASTER_DEVICE),
	m_Display (&m_I2CMaster, RESET_PIN, I2C_SLAVE_ADDRESS)
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
		bOK = m_I2CMaster.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Display.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);
	m_Logger.Write (FromKernel, LogNotice, "SSD1309 I2C display test started");

	// Test 1: Clear display (should be blank after init)
	m_Logger.Write (FromKernel, LogNotice, "Test 1: Display cleared (should be blank)");
	m_Display.Clear (0);
	CTimer::SimpleMsDelay (2000);

	// Test 2: Fill display white
	m_Logger.Write (FromKernel, LogNotice, "Test 2: Fill display white");
	m_Display.Clear (1);
	CTimer::SimpleMsDelay (2000);

	// Test 3: Clear again
	m_Display.Clear (0);
	CTimer::SimpleMsDelay (500);

	// Test 4: Draw individual pixels - horizontal line at y=0
	m_Logger.Write (FromKernel, LogNotice, "Test 3: Drawing horizontal line at y=0");
	for (unsigned x = 0; x < 128; x++)
	{
		m_Display.SetPixel (x, 0, 1);
	}
	CTimer::SimpleMsDelay (1000);

	// Test 5: Draw vertical line at x=0
	m_Logger.Write (FromKernel, LogNotice, "Test 4: Drawing vertical line at x=0");
	for (unsigned y = 0; y < 64; y++)
	{
		m_Display.SetPixel (0, y, 1);
	}
	CTimer::SimpleMsDelay (1000);

	// Test 6: Draw border rectangle
	m_Logger.Write (FromKernel, LogNotice, "Test 5: Drawing border rectangle");
	m_Display.Clear (0);
	for (unsigned x = 0; x < 128; x++)
	{
		m_Display.SetPixel (x, 0, 1);
		m_Display.SetPixel (x, 63, 1);
	}
	for (unsigned y = 0; y < 64; y++)
	{
		m_Display.SetPixel (0, y, 1);
		m_Display.SetPixel (127, y, 1);
	}
	CTimer::SimpleMsDelay (2000);

	// Test 7: Draw diagonal lines
	m_Logger.Write (FromKernel, LogNotice, "Test 6: Drawing diagonal lines");
	m_Display.Clear (0);
	for (unsigned i = 0; i < 64; i++)
	{
		m_Display.SetPixel (i * 2, i, 1);		// top-left to bottom-right
		m_Display.SetPixel (127 - i * 2, i, 1);	// top-right to bottom-left
	}
	CTimer::SimpleMsDelay (2000);

	// Test 8: Checkerboard pattern using SetArea
	m_Logger.Write (FromKernel, LogNotice, "Test 7: Checkerboard pattern");
	m_Display.Clear (0);
	{
		// Create a small 8x8 checkerboard tile
		u8 TileData[8];	// 8 rows, 1 byte per row (8 pixels wide)
		for (unsigned row = 0; row < 8; row++)
		{
			TileData[row] = (row % 2 == 0) ? 0xAA : 0x55;
		}

		// Tile it across the display
		for (unsigned ty = 0; ty < 64; ty += 8)
		{
			for (unsigned tx = 0; tx < 128; tx += 8)
			{
				CDisplay::TArea Area;
				Area.x1 = tx;
				Area.y1 = ty;
				Area.x2 = tx + 7;
				Area.y2 = ty + 7;
				m_Display.SetArea (Area, TileData);
			}
		}
	}
	CTimer::SimpleMsDelay (2000);

	// Test 9: Display on/off cycle
	m_Logger.Write (FromKernel, LogNotice, "Test 8: Display off/on cycle");
	m_Display.Off ();
	CTimer::SimpleMsDelay (1000);
	m_Display.On ();
	CTimer::SimpleMsDelay (1000);

	// Test 10: Final - show all tests passed
	m_Logger.Write (FromKernel, LogNotice, "All SSD1309 display tests completed!");

	// Draw a filled center box as final visual confirmation
	m_Display.Clear (0);
	for (unsigned y = 20; y < 44; y++)
	{
		for (unsigned x = 40; x < 88; x++)
		{
			m_Display.SetPixel (x, y, 1);
		}
	}

	m_Logger.Write (FromKernel, LogNotice,
			"Center box drawn. Display should show a white rectangle.");

	return ShutdownHalt;
}
