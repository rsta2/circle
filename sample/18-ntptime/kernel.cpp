//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2015  R. Stange <rsta2@o2online.de>
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
#include <circle/net/dnsclient.h>
#include <circle/net/ntpclient.h>
#include <circle/net/ipaddress.h>

// Network configuration
static const u8 IPAddress[]      = {192, 168, 0, 250};
static const u8 NetMask[]        = {255, 255, 255, 0};
static const u8 DefaultGateway[] = {192, 168, 0, 1};
static const u8 DNSServer[]      = {192, 168, 0, 1};

// Time configuration
static const char NTPServer[]    = "pool.ntp.org";
static const int nTimeZone       = 0*60;		// minutes diff to UTC

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_DWHCI (&m_Interrupt, &m_Timer),
	m_Net (IPAddress, NetMask, DefaultGateway, DNSServer),
	m_nTicksNextUpdate (0)
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
		bOK = m_DWHCI.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Net.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	// wait for Ethernet PHY to come up
	m_Timer.MsDelay (2000);

	m_Logger.Write (FromKernel, LogNotice, "Try \"ping %u.%u.%u.%u\" from another computer!",
			(unsigned) IPAddress[0], (unsigned) IPAddress[1],
			(unsigned) IPAddress[2], (unsigned) IPAddress[3]);

	for (unsigned nCount = 0; 1; nCount++)
	{
		if ((int) (m_nTicksNextUpdate - m_Timer.GetTicks ()) <= 0)
		{
			UpdateTime ();
		}

		m_Net.Process ();

		m_Screen.Rotor (0, nCount);
	}

	return ShutdownHalt;
}

void CKernel::UpdateTime (void)
{
	CIPAddress NTPServerIP;
	CDNSClient DNSClient (&m_Net);
	if (!DNSClient.Resolve (NTPServer, &NTPServerIP))
	{
		m_Logger.Write (FromKernel, LogWarning, "Cannot resolve: %s", NTPServer);

		m_nTicksNextUpdate = m_Timer.GetTicks () + 300 * HZ;

		return;
	}
	
	CNTPClient NTPClient (&m_Net);
	NTPClient.SetTimeZone (nTimeZone);
	unsigned nTime = NTPClient.GetTime (NTPServerIP);
	if (nTime == 0)
	{
		m_Logger.Write (FromKernel, LogWarning, "Cannot get time from %s", NTPServer);

		m_nTicksNextUpdate = m_Timer.GetTicks () + 300 * HZ;

		return;
	}

	m_Timer.SetTime (nTime);

	m_Logger.Write (FromKernel, LogNotice, "System time updated");

	m_nTicksNextUpdate = m_Timer.GetTicks () + 900 * HZ;
}
