//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2025  R. Stange <rsta2@gmx.net>
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
#include <circle/net/socket.h>
#include <circle/net/in.h>
#include <circle/string.h>
#include <circle/debug.h>

// Network configuration
#define USE_DHCP

#ifndef USE_DHCP
static const u8 IPAddress[]      = {192, 168, 0, 250};
static const u8 NetMask[]        = {255, 255, 255, 0};
static const u8 DefaultGateway[] = {192, 168, 0, 1};
static const u8 DNSServer[]      = {192, 168, 0, 1};
#endif

#define MDNS_HOST_GROUP		{224, 0, 0, 251}
#define MDNS_PORT		5353

#ifdef USE_WLAN
#define DRIVE		"SD:"
#define FIRMWARE_PATH	DRIVE "/firmware/"		// firmware files must be provided here
#define CONFIG_FILE	DRIVE "/wpa_supplicant.conf"
#endif

LOGMODULE ("kernel");

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer)
#ifdef USE_WLAN
	, m_EMMC (&m_Interrupt, &m_Timer, &m_ActLED),
	m_WLAN (FIRMWARE_PATH),
	m_Net (0, 0, 0, 0, DEFAULT_HOSTNAME, NetDeviceTypeWLAN),
	m_WPASupplicant (CONFIG_FILE)
#else
#ifndef USE_DHCP
	, m_Net (IPAddress, NetMask, DefaultGateway, DNSServer)
#endif
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

#ifdef USE_WLAN
	if (bOK)
	{
		bOK = m_EMMC.Initialize ();
	}

	if (bOK)
	{
		if (f_mount (&m_FileSystem, DRIVE, 1) != FR_OK)
		{
			LOGERR ("Cannot mount drive: %s", DRIVE);

			bOK = FALSE;
		}
	}

	if (bOK)
	{
		bOK = m_WLAN.Initialize ();
	}
#endif

	if (bOK)
	{
		bOK = m_Net.Initialize (FALSE);
	}

#ifdef USE_WLAN
	if (bOK)
	{
		bOK = m_WPASupplicant.Initialize ();
	}
#endif

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	LOGNOTE ("Compile time: " __DATE__ " " __TIME__);

	while (!m_Net.IsRunning ())
	{
		m_Scheduler.MsSleep (100);
	}

	CSocket Socket (&m_Net, IPPROTO_UDP);
	if (Socket.Bind (MDNS_PORT) < 0)
	{
		LOGPANIC ("Cannot bind to port %u", MDNS_PORT);
	}

	static const u8 mDNSIPAddress[] = MDNS_HOST_GROUP;
	CIPAddress mDNSIP (mDNSIPAddress);

#if 0
	// necessary for multicast send only
	if (Socket.Connect (mDNSIP, MDNS_PORT) < 0)
	{
		LOGPANIC ("Cannot connect to mDNS host group");
	}
#endif

	if (Socket.SetOptionAddMembership (mDNSIP) < 0)
	{
		LOGPANIC ("Cannot join mDNS host group");
	}

	while (1)
	{
		u8 Buffer[FRAME_BUFFER_SIZE];
		CIPAddress ForeignIP;
		u16 usForeignPort;
		int nResult = Socket.ReceiveFrom (Buffer, sizeof Buffer, 0,
						  &ForeignIP, &usForeignPort);
		if (nResult > 0)
		{
			CString IPString;
			ForeignIP.Format (&IPString);

			LOGNOTE ("%d bytes received from %s:%u",
				 nResult, IPString.c_str (), (unsigned) usForeignPort);

#ifndef NDEBUG
			debug_hexdump (Buffer, nResult, From);
#endif
		}
		else if (nResult < 0)
		{
			LOGWARN ("Receive failed");
		}

		m_Scheduler.Yield ();
	}

	return ShutdownHalt;
}
