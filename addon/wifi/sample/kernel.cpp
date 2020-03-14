//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#include <circle/net/ntpdaemon.h>

// network to join
#define ESSID		"TEST"
#define AUTH_MODE	CBcm4343Device::AuthModeNone
#define AUTH_KEY	"mykey"

// firmware files must be provided here
#define FIRMWARE_DRIVE	"USB:"
#define FIRMWARE_PATH	FIRMWARE_DRIVE "/firmware/"

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer),
	m_WiFi (FIRMWARE_PATH)
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
		if (f_mount (&m_FileSystem, FIRMWARE_DRIVE, 1) != FR_OK)
		{
			m_Logger.Write (FromKernel, LogError,
					"Cannot mount drive: %s", FIRMWARE_DRIVE);

			bOK = FALSE;
		}
	}

	if (bOK)
	{
		m_WiFi.SetESSID (ESSID);
		m_WiFi.SetAuth (AUTH_MODE, AUTH_KEY);

		bOK = m_WiFi.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Net.Initialize (FALSE);
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	m_WiFi.DumpStatus ();

	m_Scheduler.Sleep (5);

	m_Timer.SetTimeZone (0*60);
	new CNTPDaemon ("pool.ntp.org", &m_Net);

	m_Scheduler.Sleep (20);

	return ShutdownHalt;
}
