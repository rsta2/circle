//
// netsocket.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018  R. Stange <rsta2@o2online.de>
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
#include <circle/net/netsocket.h>
#include <circle/net/netsubsystem.h>
#include <circle/net/dnsclient.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <assert.h>

static const char FromNetSocket[] = "netsocket";

CNetSocket::CNetSocket (CNetSubSystem *pNetSubSystem)
:	m_pNetSubSystem (pNetSubSystem)
{
	assert (m_pNetSubSystem != 0);
}

CNetSocket::~CNetSocket (void)
{
	m_pNetSubSystem = 0;
}

int CNetSocket::Connect (const char *pHost, const char *pPort)
{
	assert (pHost != 0);
	assert (pPort != 0);

	char *pEnd = 0;
	unsigned long ulPort = strtoul (pPort, &pEnd, 10);
	if (   (pEnd != 0 && *pEnd != '\0')
	    || (ulPort == 0 || ulPort > 0xFFFF))
	{
		CLogger::Get ()->Write (FromNetSocket, LogDebug, "Invalid port number: %s", pPort);

		return -1;
	}

	assert (m_pNetSubSystem != 0);
	CIPAddress IPAddress;
	CDNSClient DNSClient (m_pNetSubSystem);
	if (!DNSClient.Resolve (pHost, &IPAddress))
	{
		CLogger::Get ()->Write (FromNetSocket, LogDebug, "Cannot resolve: %s", pHost);

		return -1;
	}

	return Connect (IPAddress, static_cast<u16> (ulPort));
}

CNetSubSystem *CNetSocket::GetNetSubSystem (void)
{
	assert (m_pNetSubSystem != 0);
	return m_pNetSubSystem;
}
