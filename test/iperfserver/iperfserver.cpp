//
// iperfserver.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2018  R. Stange <rsta2@o2online.de>
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
#include "iperfserver.h"
#include <circle/net/in.h>
#include <circle/logger.h>
#include <circle/timer.h>
#include <circle/string.h>
#include <assert.h>

static const char FromIPerf[] = "iperf";

unsigned CIPerfServer::s_nInstanceCount = 0;

CIPerfServer::CIPerfServer (CNetSubSystem *pNetSubSystem, CSocket *pSocket,
			    const CIPAddress *pClientIP, u16 usClientPort)
:	m_pNetSubSystem (pNetSubSystem),
	m_pSocket (pSocket),
	m_usClientPort (usClientPort)
{
	s_nInstanceCount++;

	if (pClientIP != 0)
	{
		m_ClientIP.Set (*pClientIP);
	}
}

CIPerfServer::~CIPerfServer (void)
{
	assert (m_pSocket == 0);

	m_pNetSubSystem = 0;

	s_nInstanceCount--;
}

void CIPerfServer::Run (void)
{
	if (m_pSocket == 0)
	{
		Listener ();
	}
	else
	{
		Worker ();
	}
}

void CIPerfServer::Listener (void)
{
	assert (m_pNetSubSystem != 0);
	m_pSocket = new CSocket (m_pNetSubSystem, IPPROTO_TCP);
	assert (m_pSocket != 0);

	if (m_pSocket->Bind (IPERF_PORT) < 0)
	{
		CLogger::Get ()->Write (FromIPerf, LogError, "Cannot bind socket (port %u)", IPERF_PORT);

		delete m_pSocket;
		m_pSocket = 0;

		return;
	}

	if (m_pSocket->Listen (MAX_CLIENTS) < 0)
	{
		CLogger::Get ()->Write (FromIPerf, LogError, "Cannot listen on socket");

		delete m_pSocket;
		m_pSocket = 0;

		return;
	}

	while (1)
	{
		CIPAddress ForeignIP;
		u16 usForeignPort;
		CSocket *pConnection = m_pSocket->Accept (&ForeignIP, &usForeignPort);
		if (pConnection == 0)
		{
			CLogger::Get ()->Write (FromIPerf, LogWarning, "Cannot accept connection");

			continue;
		}

		CString IPString;
		ForeignIP.Format (&IPString);
		CLogger::Get ()->Write (FromIPerf, LogNotice, "%s:%u: Incoming connection",
					(const char *) IPString, (unsigned) usForeignPort);

		if (s_nInstanceCount >= MAX_CLIENTS+1)
		{
			CLogger::Get ()->Write (FromIPerf, LogWarning, "Too many clients");

			delete pConnection;

			continue;
		}

		new CIPerfServer (m_pNetSubSystem, pConnection, &ForeignIP, usForeignPort);
	}
}

void CIPerfServer::Worker (void)
{
	assert (m_pSocket != 0);

	u64 ullTotalBytesReceived = 0;
	boolean bStarted = FALSE;
	unsigned nStartTicks = 0;

	u8 Buffer[FRAME_BUFFER_SIZE];
	int nBytesReceived;

	do
	{
		nBytesReceived = m_pSocket->Receive (Buffer, sizeof Buffer, 0);
		if (nBytesReceived > 0)
		{
			if (!bStarted)
			{
				nStartTicks = CTimer::Get ()->GetClockTicks ();

				bStarted = TRUE;
			}

			ullTotalBytesReceived += nBytesReceived;
		}
	}
	while (nBytesReceived > 0);

	unsigned nEndTicks = CTimer::Get ()->GetClockTicks ();

	delete m_pSocket;		// closes connection
	m_pSocket = 0;

	CString IPString;
	m_ClientIP.Format (&IPString);

	if (!bStarted)
	{
		CLogger::Get ()->Write (FromIPerf, LogNotice, "%s:%u: No data received",
					(const char *) IPString, (unsigned) m_usClientPort);

		return;
	}

	float fSeconds = (float) (nEndTicks - nStartTicks) / CLOCKHZ;
	float fMBytes = (float) ullTotalBytesReceived / 1048576.0;
	float fMBitsPerSec = fMBytes*8.0 / fSeconds;

	fMBitsPerSec *= (1516.0 + 36.0) / 1480.0;	// consider protocol overhead

	CLogger::Get ()->Write (FromIPerf, LogNotice,
				"%s:%u: Received %.1f MBytes in %.1f sec (bandwidth %.1f MBits/sec)",
				(const char *) IPString, (unsigned) m_usClientPort,
				fMBytes+0.05, fSeconds+0.05, fMBitsPerSec+0.05);
}
