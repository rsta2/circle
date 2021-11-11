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
#include <circle/string.h>

//#define USE_OPEN_NET	"TEST"				// SSID

#define DRIVE		"SD:"
#define FIRMWARE_PATH	DRIVE "/firmware/"		// firmware files must be provided here
#define CONFIG_FILE	DRIVE "/wpa_supplicant.conf"

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer),
	m_EMMC (&m_Interrupt, &m_Timer, &m_ActLED),
	m_WLAN (FIRMWARE_PATH),
	m_Net (0, 0, 0, 0, DEFAULT_HOSTNAME, NetDeviceTypeWLAN),
	m_WPASupplicant (CONFIG_FILE)
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
		bOK = m_EMMC.Initialize ();
	}

	if (bOK)
	{
		if (f_mount (&m_FileSystem, DRIVE, 1) != FR_OK)
		{
			m_Logger.Write (FromKernel, LogError,
					"Cannot mount drive: %s", DRIVE);

			bOK = FALSE;
		}
	}

	if (bOK)
	{
		bOK = m_WLAN.Initialize ();
	}

#ifdef USE_OPEN_NET
	if (bOK)
	{
		bOK = m_WLAN.JoinOpenNet (USE_OPEN_NET);
	}
#endif

	if (bOK)
	{
		bOK = m_Net.Initialize (FALSE);
	}

#ifndef USE_OPEN_NET
	if (bOK)
	{
		bOK = m_WPASupplicant.Initialize ();
	}
#endif

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	while (!m_Net.IsRunning ())
	{
		m_Scheduler.MsSleep (100);
	}

	//m_WLAN.DumpStatus ();

	CString IPString;
	m_Net.GetConfig ()->GetIPAddress ()->Format (&IPString);
	m_Logger.Write (FromKernel, LogNotice, "Try \"ping %s\" from another computer!",
			(const char *) IPString);

	m_Timer.SetTimeZone (0*60);
	new CNTPDaemon ("pool.ntp.org", &m_Net);

	while (1)
	{
		m_Scheduler.Sleep (10);
	}

	return ShutdownHalt;
}
