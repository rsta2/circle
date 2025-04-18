//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2024  R. Stange <rsta2@o2online.de>
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
#include "pingclient.h"
#include <circle/string.h>

// Network configuration
static const u8 PingServer[]     = {192, 168, 0, 1};

#define USE_DHCP

#ifndef USE_DHCP
static const u8 IPAddress[]      = {192, 168, 0, 250};
static const u8 NetMask[]        = {255, 255, 255, 0};
static const u8 DefaultGateway[] = {192, 168, 0, 1};
static const u8 DNSServer[]      = {192, 168, 0, 1};
#endif

LOGMODULE ("kernel");

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
	LOGNOTE ("Compile time: " __DATE__ " " __TIME__);

	CPingClient Ping (&m_Net);

	unsigned nLastSend = 0;
	unsigned nLastSendTicks = 0;
	unsigned nSequenceNumber = 1;
	for (unsigned nCount = 0; 1; nCount++)
	{
		const u16 Identifier = 1;

		unsigned nTime = m_Timer.GetUptime ();
		if (nTime - nLastSend >= 1)
		{
			nLastSend = nTime;
			nLastSendTicks = CTimer::GetClockTicks ();

			CIPAddress Server (PingServer);
			if (!Ping.Send (Server, Identifier, nSequenceNumber++))
			{
				LOGWARN ("Send failed");
			}
		}

		CIPAddress SenderIP;
		u16 usIdentifier, usSequenceNumber;
		size_t ulLength;
		if (Ping.Receive (&SenderIP, &usIdentifier, &usSequenceNumber, &ulLength))
		{
			if (   SenderIP == PingServer
			    && usIdentifier == Identifier)
			{
				CString Sender;
				SenderIP.Format (&Sender);

				LOGNOTE ("%lu bytes from %s: icmp_seq=%u time=%.2f ms",
					 ulLength,
					 (const char *) Sender,
					 (unsigned) usSequenceNumber,
					 (CTimer::GetClockTicks () - nLastSendTicks) * 1000.0 / CLOCKHZ);
			}
		}

		m_Scheduler.Yield ();

		m_Screen.Rotor (0, nCount);
	}

	return ShutdownHalt;
}
