//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020  H. Kocevar <hinxx@protonmail.com>
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
#include <circle/usb/usbserialch341.h>
#include <circle/string.h>

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer)
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

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	CUSBSerialCH341Device *pUSerial1 = reinterpret_cast<CUSBSerialCH341Device *>(m_DeviceNameService.GetDevice ("utty1", FALSE));
	if (pUSerial1 == 0)
	{
		m_Logger.Write (FromKernel, LogPanic, "USB serial device not found");
	}

	pUSerial1->SetBaudRate(230400);

	if (pUSerial1->Write ("vO", 2) < 0)
	{
		m_Logger.Write (FromKernel, LogError, "Write error vO");
	}
	m_Logger.Write (FromKernel, LogDebug, "Write vO");
	if (pUSerial1->Write ("vN", 2) < 0)
	{
		m_Logger.Write (FromKernel, LogError, "Write error vN");
	}
	m_Logger.Write (FromKernel, LogDebug, "Write vN");

	int iter = 20;
	while (iter--)
	{
		if (pUSerial1->Write ("v0", 2) < 0)
		{
			m_Logger.Write (FromKernel, LogError, "Write error v0");
		}
		m_Logger.Write (FromKernel, LogDebug, "Write v0");
		m_Timer.SimpleMsDelay(500);
		if (pUSerial1->Write ("v1", 2) < 0)
		{
			m_Logger.Write (FromKernel, LogError, "Write error v1");
		}
		m_Logger.Write (FromKernel, LogDebug, "Write v1");
		m_Timer.SimpleMsDelay(500);
	}

	if (pUSerial1->Write ("uI", 2) < 0)
	{
		m_Logger.Write (FromKernel, LogError, "Write error uI");
	}
	m_Logger.Write (FromKernel, LogDebug, "Write uI");
	if (pUSerial1->Write ("uN", 2) < 0)
	{
		m_Logger.Write (FromKernel, LogError, "Write error uN");
	}
	m_Logger.Write (FromKernel, LogDebug, "Write uN");

	u8 rxData[5];
	iter = 20;
	while (iter--)
	{
		if (pUSerial1->Write ("u?", 2) < 0)
		{
			m_Logger.Write (FromKernel, LogError, "Write error u?");
		}
		m_Logger.Write (FromKernel, LogDebug, "Write u?");
		m_Timer.SimpleMsDelay(500);
		if (pUSerial1->Read (rxData, 4) < 0)
		{
			m_Logger.Write (FromKernel, LogError, "Read error");
		}
		m_Logger.Write (FromKernel, LogDebug, "Read u? %2X %2X %2X %2X", rxData[0], rxData[1], rxData[2], rxData[3]);
		m_Timer.SimpleMsDelay(500);
	}

	return ShutdownReboot;
}
