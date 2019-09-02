//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2017  R. Stange <rsta2@o2online.de>
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
#include <circle/net/syslogdaemon.h>
#include <circle/net/ipaddress.h>
#include <assert.h>

// Syslog configuration
static const u8 SysLogServer[]   = {192, 168, 0, 158};
static const u16 usServerPort    = 8514;		// standard port is 514

// Time configuration
#define USE_NTP

#ifdef USE_NTP
static const char NTPServer[]    = "pool.ntp.org";
static const int nTimeZone       = 0*60;		// minutes diff to UTC
#endif

// Network configuration
#define USE_DHCP

#ifndef USE_DHCP
static const u8 IPAddress[]      = {192, 168, 0, 250};
static const u8 NetMask[]        = {255, 255, 255, 0};
static const u8 DefaultGateway[] = {192, 168, 0, 1};
static const u8 DNSServer[]      = {192, 168, 0, 1};
#endif

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer)
#ifndef USE_DHCP
	, m_Net (IPAddress, NetMask, DefaultGateway, DNSServer)
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

	if (bOK)
	{
		bOK = m_Net.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

#ifdef USE_NTP
	m_Timer.SetTimeZone (nTimeZone);
	new CNTPDaemon (NTPServer, &m_Net);
#endif

	CIPAddress ServerIP (SysLogServer);
	CString IPString;
	ServerIP.Format (&IPString);
	m_Logger.Write (FromKernel, LogNotice, "Sending log messages to %s:%u",
			(const char *) IPString, (unsigned) usServerPort);

	new CSysLogDaemon (&m_Net, ServerIP, usServerPort);

	for (unsigned i = 1; i <= 10; i++)
	{
		m_Scheduler.Sleep (5);

		m_Logger.Write (FromKernel, LogNotice, "Hello syslog! (%u)", i);
	}

	m_Logger.Write (FromKernel, LogPanic, "System will halt now");

	return ShutdownHalt;
}
