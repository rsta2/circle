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
#include <circle/string.h>
#include <circle/usb/usbserialch341.h>
#include <gpio/rtkgpio.h>

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

	CUSBSerialCH341Device *pUSerial1 = (CUSBSerialCH341Device *) (m_DeviceNameService.GetDevice ("utty1", FALSE));
	if (pUSerial1 == 0)
	{
		m_Logger.Write (FromKernel, LogPanic, "USB serial device not found");
	}

	CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
	if (pTarget == 0)
	{
		pTarget = &m_Screen;
	}

	CRTKGpioDevice rtkgpio (pUSerial1);
	rtkgpio.Initialize ();

	for (unsigned i = 22; i < 26; i++)
	{
		rtkgpio.SetPinDirectionInput (i);
		rtkgpio.SetPinPullNone (i);
	}

	for (unsigned i = 0; i < 8; i++)
	{
		rtkgpio.SetPinDirectionOutput (i);
		rtkgpio.SetPinPullNone (i);
		rtkgpio.SetPinLevelHigh (i);
	}

	CString status;

	for (unsigned i = 0; i < 256; i++)
	{
		status.Append ("\routputs:");
		for (unsigned b = 0; b < 8; b++)
		{
			if (!(i & (1 << b)))
			{
				rtkgpio.SetPinLevelHigh (b);
				status.Append (" H");
			}
			else
			{
				rtkgpio.SetPinLevelLow (b);
				status.Append (" L");
			}
		}
		status.Append ("\tinputs:");
		for (unsigned b = 22; b < 26; b++)
		{
			if (rtkgpio.GetPinLevel (b) == RtkGpioLevelLow)
			{
				status.Append (" L");
			}
			else
			{
				status.Append (" H");
			}
		}

		pTarget->Write (status, status.GetLength ());
		CTimer::SimpleMsDelay (500);
	}

	m_Logger.Write (FromKernel, LogNotice, "\nDone!");

	return ShutdownReboot;
}
