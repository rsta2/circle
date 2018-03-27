//
// echoserver.cpp
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
#include "echoserver.h"
#include <circle/net/in.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <assert.h>

static const char FromEcho[] = "echo";

unsigned CEchoServer::s_nInstanceCount = 0;

CEchoServer::CEchoServer (CNetSubSystem *pNetSubSystem, CSocket *pSocket, const CIPAddress *pClientIP)
:	m_pNetSubSystem (pNetSubSystem),
	m_pSocket (pSocket)
{
	s_nInstanceCount++;

	if (pClientIP != 0)
	{
		m_ClientIP.Set (*pClientIP);
	}
}

CEchoServer::~CEchoServer (void)
{
	assert (m_pSocket == 0);

	m_pNetSubSystem = 0;

	s_nInstanceCount--;
}

void CEchoServer::Run (void)
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

void CEchoServer::Listener (void)
{
	assert (m_pNetSubSystem != 0);
	m_pSocket = new CSocket (m_pNetSubSystem, IPPROTO_TCP);
	assert (m_pSocket != 0);

	if (m_pSocket->Bind (ECHO_PORT) < 0)
	{
		CLogger::Get ()->Write (FromEcho, LogError, "Cannot bind socket (port %u)", ECHO_PORT);

		delete m_pSocket;
		m_pSocket = 0;

		return;
	}

	if (m_pSocket->Listen (MAX_CLIENTS) < 0)
	{
		CLogger::Get ()->Write (FromEcho, LogError, "Cannot listen on socket");

		delete m_pSocket;
		m_pSocket = 0;

		return;
	}

	while (1)
	{
		CIPAddress ForeignIP;
		u16 nForeignPort;
		CSocket *pConnection = m_pSocket->Accept (&ForeignIP, &nForeignPort);
		if (pConnection == 0)
		{
			CLogger::Get ()->Write (FromEcho, LogWarning, "Cannot accept connection");

			continue;
		}

		CString IPString;
		ForeignIP.Format (&IPString);
		CLogger::Get ()->Write (FromEcho, LogNotice, "Incoming connection from %s", (const char *) IPString);

		if (s_nInstanceCount >= MAX_CLIENTS+1)
		{
			CLogger::Get ()->Write (FromEcho, LogWarning, "Too many clients");

			delete pConnection;

			continue;
		}

		new CEchoServer (m_pNetSubSystem, pConnection, &ForeignIP);
	}
}

void CEchoServer::Worker (void)
{
	assert (m_pSocket != 0);

	u8 Buffer[FRAME_BUFFER_SIZE];
	int nBytesReceived;

	do
	{
		nBytesReceived = m_pSocket->Receive (Buffer, sizeof Buffer, 0);
		if (nBytesReceived > 0)
		{
			if (m_pSocket->Send (Buffer, nBytesReceived, MSG_DONTWAIT) != nBytesReceived)
			{
				CLogger::Get ()->Write (FromEcho, LogWarning, "Cannot send echo");

				break;
			}
		}
	}
	while (nBytesReceived > 0);

	CString IPString;
	m_ClientIP.Format (&IPString);
	CLogger::Get ()->Write (FromEcho, LogNotice, "Closing connection with %s", (const char *) IPString);

	delete m_pSocket;		// closes connection
	m_pSocket = 0;
}
