//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2022  R. Stange <rsta2@o2online.de>
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
#include "htmlscanner.h"
#include <circle/net/dnsclient.h>
#include <circle/net/httpclient.h>
#include <circle/net/ipaddress.h>
#include <circle/machineinfo.h>
#include <circle/string.h>
#include <circle/util.h>
#include <assert.h>

// Network configuration
#define USE_DHCP

#ifndef USE_DHCP
static const u8 IPAddress[]      = {192, 168, 0, 250};
static const u8 NetMask[]        = {255, 255, 255, 0};
static const u8 DefaultGateway[] = {192, 168, 0, 1};
static const u8 DNSServer[]      = {192, 168, 0, 1};
#endif

// Server configuration
static const char Server[]        = "elinux.org";
static const char Document[]      = "/RPi_HardwareHistory";
static const unsigned nDocMaxSize = 200*1024;

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

	char *pBuffer = new char[nDocMaxSize+1];	// +1 for 0-termination
	if (pBuffer == 0)
	{
		CLogger::Get ()->Write (FromKernel, LogPanic, "Cannot allocate document buffer");
	}

	if (GetDocument (pBuffer))
	{
		ParseDocument (pBuffer);
	}

	delete [] pBuffer;
	pBuffer = 0;

	// should not halt here, because TCP disconnect may still be in process

	for (unsigned nCount = 0; 1; nCount++)
	{
		m_Scheduler.Yield ();

		m_Screen.Rotor (0, nCount);
	}

	return ShutdownHalt;
}

boolean CKernel::GetDocument (char *pBuffer)
{
	CIPAddress ForeignIP;
	CDNSClient DNSClient (&m_Net);
	if (!DNSClient.Resolve (Server, &ForeignIP))
	{
		CLogger::Get ()->Write (FromKernel, LogError, "Cannot resolve: %s", Server);

		return FALSE;
	}

	CString IPString;
	ForeignIP.Format (&IPString);
	m_Logger.Write (FromKernel, LogDebug, "Outgoing connection to %s", (const char *) IPString);

	unsigned nLength = nDocMaxSize;

	assert (pBuffer != 0);
	CHTTPClient Client (&m_Net, ForeignIP, HTTP_PORT, Server);
	THTTPStatus Status = Client.Get (Document, (u8 *) pBuffer, &nLength);
	if (Status != HTTPOK)
	{
		m_Logger.Write (FromKernel, LogError, "HTTP request failed (status %u)", Status);

		return FALSE;
	}

	m_Logger.Write (FromKernel, LogDebug, "%u bytes successfully received", nLength);

	assert (nLength <= nDocMaxSize);
	pBuffer[nLength] = '\0';

	return TRUE;
}

boolean CKernel::ParseDocument (const char *pDocument)
{
	CMachineInfo MachineInfo;
	u32 nBoardRevision = MachineInfo.GetRevisionRaw () & 0xFFFFFF;		// mask out warranty bits
	m_Logger.Write (FromKernel, LogNotice, "Revision of this board is %04x", nBoardRevision);

	assert (pDocument != 0);
	CHtmlScanner Scanner (pDocument);

	unsigned nState = 0;

	CString ItemText;
	THtmlItem Item = Scanner.GetFirstItem (ItemText);
	while (Item < HtmlItemEOF)
	{
		switch (nState)
		{
		case 0:		// find a HTML table
			static const char Pattern[] = "<table";
			if (   Item == HtmlItemTag
			    && strncmp (ItemText, Pattern, sizeof Pattern-1) == 0)
			{
				nState = 1;
			}
			break;

		case 1:		// is the first column header == "Revision"?
			if (Item == HtmlItemText)
			{
				if (ItemText.Compare ("Revision") == 0)
				{
					// yes, start displaying the list
					static const char Msg[] = "\n";
					m_Screen.Write (Msg, sizeof Msg-1);
					nState = 2;
				}
				else
				{
					// no, find next table
					nState = 0;
					break;
				}
			}
			else
			{
				// ignore other items
				break;
			}
			// fall through

		case 2:		// first column
		case 3:		// other column
			if (Item == HtmlItemTag)
			{
				if (ItemText.Compare ("</table>") == 0)
				{
					// table is finished, leave
					return TRUE;
				}
				else if (ItemText.Compare ("</tr>") == 0)
				{
					// reset highlighting and output newline
					static const char Msg[] = "\x1b[0m\n";
					m_Screen.Write (Msg, sizeof Msg-1);
					nState = 2;
				}
			}
			else if (Item == HtmlItemText)
			{
				if (nState == 2)
				{
					// check for valid revision number and compare it with ours
					char *pEnd;
					unsigned long ulRevision = strtoul (ItemText, &pEnd, 16);
					if (   (    pEnd == 0
						|| *pEnd == '\0')
					    && ulRevision == nBoardRevision)
					{
						// highlight this line, because it is ours
						static const char Msg[] = "\x1b[1m";
						m_Screen.Write (Msg, sizeof Msg-1);
					}
				}
				else
				{
					static const char Msg[] = ", ";
					m_Screen.Write (Msg, sizeof Msg-1);
				}

				ItemText.Replace ("&#160;", " ");

				m_Screen.Write (ItemText, ItemText.GetLength ());

				nState = 3;
			}
			break;

		default:
			assert (0);
			break;
		}

		Item = Scanner.GetNextItem (ItemText);
	}

	if (Item != HtmlItemEOF)
	{
		m_Logger.Write (FromKernel, LogError, "HTML scanner error");

		return FALSE;
	}

	m_Logger.Write (FromKernel, LogError, "Revision table not found");

	return FALSE;
}
