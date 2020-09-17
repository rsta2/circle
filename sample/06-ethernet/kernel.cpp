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
#include <circle/usb/usb.h>
#include <circle/netdevice.h>
#include <circle/macaddress.h>
#include <circle/string.h>
#include <circle/synchronize.h>
#include <circle/macros.h>
#include <circle/debug.h>

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer)
#if RASPPI <= 3
	, m_USBHCI (&m_Interrupt, &m_Timer)
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
#if RASPPI <= 3
		bOK = m_USBHCI.Initialize ();
#else
		bOK = m_Bcm54213.Initialize ();
#endif
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	CNetDevice *pEth0 = CNetDevice::GetNetDevice (0);
	if (pEth0 == 0)
	{
		m_Logger.Write (FromKernel, LogError, "Net device not found");

		return ShutdownHalt;
	}

	// wait for Ethernet PHY to come up
	m_Timer.MsDelay (2000);

	m_Logger.Write (FromKernel, LogNotice, "Dumping received broadcasts");

	while (1)
	{
		DMA_BUFFER (u8, FrameBuffer, FRAME_BUFFER_SIZE);
		unsigned nFrameLength;

		if (pEth0->ReceiveFrame (FrameBuffer, &nFrameLength))
		{
			CString Sender ("???");
			CString Protocol ("???");

			if (nFrameLength >= 14)
			{
				CMACAddress MACSender (FrameBuffer+6);
				MACSender.Format (&Sender);

				unsigned nProtocol = *(unsigned short *) (FrameBuffer+12);
				switch (nProtocol)
				{
				case BE (0x800):
					Protocol = "IP";
					break;

				case BE (0x806):
					Protocol = "ARP";
					break;

				default:
					break;
				}
			}

			m_Logger.Write (FromKernel, LogNotice,
					"%u bytes received from %s (protocol %s)", nFrameLength,
					(const char *) Sender, (const char *) Protocol);
#ifndef NDEBUG
			debug_hexdump (FrameBuffer, nFrameLength, FromKernel);
#endif
		}
	}

	return ShutdownHalt;
}
