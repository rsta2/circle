//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/net/ipaddress.h>
#include <circle/net/socket.h>
#include <circle/net/in.h>
#include <assert.h>

//#define COUNTRY_CODE	"DE"

#define SSID_OPEN_NET	"TEST"

#define DRIVE		"SD:"
#define FIRMWARE_PATH	DRIVE "/firmware/"		// firmware files must be provided here

static const u8 IPAddress[] = {192, 168, 1, 10};
static const u8 NetMask[]   = {255, 255, 255, 0};
static const u8 ServerIP[]  = {192, 168, 1, 1};

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer),
	m_EMMC (&m_Interrupt, &m_Timer, &m_ActLED),
	m_WLAN (FIRMWARE_PATH),
	m_Net (IPAddress, NetMask, 0, 0, DEFAULT_HOSTNAME, NetDeviceTypeWLAN)
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

#ifdef COUNTRY_CODE
	if (bOK)
	{
		bOK = m_WLAN.Control ("country %s", COUNTRY_CODE);
	}
#endif

	if (bOK)
	{
		bOK = m_WLAN.JoinOpenNet (SSID_OPEN_NET);
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

	//m_WLAN.DumpStatus ();

	CSocket *pSocket = new CSocket (&m_Net, IPPROTO_TCP);
	assert (pSocket != 0);

	CIPAddress Server (ServerIP);
	if (pSocket->Connect (Server, 7) < 0)
	{
		m_Logger.Write (FromKernel, LogPanic, "Connect failed");
	}

	m_Logger.Write (FromKernel, LogDebug, "Connected");

	unsigned nLastSend = 0;
	while (m_Timer.GetUptime () < 120)
	{
		u8 Buffer[FRAME_BUFFER_SIZE];
		int nBytesReceived = pSocket->Receive (Buffer, sizeof Buffer, MSG_DONTWAIT);
		if (nBytesReceived > 0)
		{
			Buffer[nBytesReceived] = '\0';
			m_Logger.Write (FromKernel, LogNotice, "Message received: %s", Buffer);
		}

		unsigned nUptime = m_Timer.GetUptime ();
		if (nUptime - nLastSend >= 5)
		{
			static const char Msg[] = "Hello AP!";
			if (pSocket->Send (Msg, sizeof Msg-1, MSG_DONTWAIT) != sizeof Msg-1)
			{
				CLogger::Get ()->Write (FromKernel, LogWarning, "Cannot send");
			}

			nLastSend = nUptime;

			m_Logger.Write (FromKernel, LogDebug, "Message sent: %s", Msg);
		}

		m_Scheduler.Yield ();
	}

	delete pSocket;

	m_Logger.Write (FromKernel, LogDebug, "Disconnected");

	while (1)
	{
		m_Scheduler.Sleep (10);
	}

	return ShutdownHalt;
}
