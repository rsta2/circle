//
// ntpclient.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>
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
#include <circle/net/ntpclient.h>
#include <circle/net/socket.h>
#include <circle/net/in.h>
#include <circle/sched/scheduler.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <circle/types.h>
#include <assert.h>

#define NTP_PACKET_SIZE		48

#define SEVENTY_YEARS		2208988800U

static const char FromNTPClient[] = "ntp";

CNTPClient::CNTPClient (CNetSubSystem *pNetSubSystem)
:	m_pNetSubSystem (pNetSubSystem)
{
	assert (m_pNetSubSystem != 0);
}

CNTPClient::~CNTPClient (void)
{
	m_pNetSubSystem = 0;
}

unsigned CNTPClient::GetTime (CIPAddress &rServerIP)
{
	assert (m_pNetSubSystem != 0);
	CSocket Socket (m_pNetSubSystem, IPPROTO_UDP);
	if (Socket.Connect (rServerIP, 123) != 0)
	{
		return 0;
	}

	u8 NTPPacket[NTP_PACKET_SIZE];
	memset (NTPPacket, 0, sizeof NTPPacket);
	NTPPacket[0] = 0xE3;			// leap indicator: unknown, version: 4, mode: client
	NTPPacket[1] = 0;			// stratum: unspecified
	NTPPacket[2] = 10;			// poll: 1024 seconds
	NTPPacket[3] = 0;			// precision: 1 second
	memcpy (NTPPacket+12, "XCIR", 4);

	unsigned char RecvPacket[NTP_PACKET_SIZE];

	unsigned nTry;
	for (nTry = 1; nTry <= 3; nTry++)
	{
		if (Socket.Send (NTPPacket, sizeof NTPPacket, 0) != sizeof NTPPacket)
		{
			CLogger::Get ()->Write (FromNTPClient, LogError, "Send failed");

			return 0;
		}

		CScheduler::Get ()->MsSleep (1000);

		int nResult = Socket.Receive (RecvPacket, sizeof RecvPacket, MSG_DONTWAIT);
		if (nResult < 0)
		{
			CLogger::Get ()->Write (FromNTPClient, LogError, "Receive failed");

			return 0;
		}

		if (nResult >= 44)
		{
			break;
		}
	}

	if (nTry > 3)
	{
		CLogger::Get ()->Write (FromNTPClient, LogError, "Invalid or no response");

		return 0;
	}

	unsigned nSecondsSince1900;
	nSecondsSince1900  =   (unsigned) RecvPacket[40] << 24
			     | (unsigned) RecvPacket[41] << 16
			     | (unsigned) RecvPacket[42] << 8
			     | (unsigned) RecvPacket[43];

	if (nSecondsSince1900 < SEVENTY_YEARS)
	{
		return 0;
	}

	unsigned nTime = nSecondsSince1900 - SEVENTY_YEARS;
	
	return nTime;
}
