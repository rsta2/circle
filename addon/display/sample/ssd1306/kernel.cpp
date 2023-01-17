//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2018  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbkeyboard.h>
#include <circle/input/keyboardbuffer.h>
#include <circle/time.h>
#include <circle/string.h>
#include <circle/util.h>
#include <circle/machineinfo.h>

// LCD configuration
#define HEIGHT	32
#define WIDTH	128

#define I2C_ADDR 0x3C

#define I2C_MASTER_DEVICE	(CMachineInfo::Get ()->GetDevice (DeviceI2CMaster))

static const char FromKernel[] = "kernel";

// Set to true to enable rotated or mirrored displays
#define ROTATED   false
#define MIRRORED  false

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer),
	m_I2CMaster (I2C_MASTER_DEVICE),
	m_LCD (WIDTH, HEIGHT, &m_I2CMaster, I2C_ADDR, ROTATED, MIRRORED)
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
	
	if (bOK)
	{
		bOK = m_I2CMaster.Initialize ();
	}

	if (bOK)
	{
		bOK = m_LCD.Initialize ();
	}
	
	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	LCDWrite ("01234567890123456789");
	LCDWrite (">>>>>>>>>>>>>>>>>>>");

	unsigned nTime = 10 + m_Timer.GetTime ();
	while (nTime > m_Timer.GetTime ())
	{
		// just wait a few seconds
	}
	
	LCDWrite ("\E[H\E[J"); // Reset cursor and clear display

	CUSBKeyboardDevice *pKeyboard =
		(CUSBKeyboardDevice *) m_DeviceNameService.GetDevice ("ukbd1", FALSE);
	if (pKeyboard == 0)
	{
		m_Logger.Write (FromKernel, LogError, "Keyboard not found");

		TimeDemo ();

		return ShutdownHalt;
	}

	CKeyboardBuffer Keyboard (pKeyboard);

	m_Logger.Write (FromKernel, LogNotice, "Just type something!");
	LCDWrite ("Just type!\n");

	for (unsigned nCount = 0; 1; nCount++)
	{
		char Buffer[10];
		int nResult = Keyboard.Read (Buffer, sizeof Buffer);
		if (nResult > 0)
		{
			m_LCD.Write (Buffer, nResult);
		}

		m_Screen.Rotor (0, nCount);
	}

	return ShutdownHalt;
}

void CKernel::TimeDemo (void)
{
	LCDWrite ("\x1B[?25l");		// cursor off
	LCDWrite ("No keyboard!\n");

	unsigned nTime = m_Timer.GetTime ();
	while (1)
	{
		while (nTime == m_Timer.GetTime ())
		{
			// just wait a second
		}

		nTime = m_Timer.GetTime ();

		CTime Time;
		Time.Set (nTime);

		CString String;
		String.Format ("\r%02u:%02u:%02u",
			Time.GetHours (), Time.GetMinutes (), Time.GetSeconds ());

		LCDWrite ((const char *) String);
	}
}

void CKernel::LCDWrite (const char *pString)
{
	m_LCD.Write (pString, strlen (pString));
}
