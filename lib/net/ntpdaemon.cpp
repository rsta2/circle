//
// ntpdaemon.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/net/ntpdaemon.h>
#include <circle/net/dnsclient.h>
#include <circle/net/ntpclient.h>
#include <circle/net/ipaddress.h>
#include <circle/sched/scheduler.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <assert.h>

static const char FromNTPDaemon[] = "ntpd";

CNTPDaemon::CNTPDaemon (const char *pNTPServer, CNetSubSystem *pNetSubSystem)
:	m_NTPServer (pNTPServer),
	m_pNetSubSystem (pNetSubSystem)
{
	assert (m_pNetSubSystem != 0);

	SetName (FromNTPDaemon);
}

CNTPDaemon::~CNTPDaemon (void)
{
	m_pNetSubSystem = 0;
}

void CNTPDaemon::Run (void)
{
	while (1)
	{
		unsigned nSecondsToNextAttempt = UpdateTime ();
		
		CScheduler::Get ()->Sleep (nSecondsToNextAttempt);
	}
}

unsigned CNTPDaemon::UpdateTime (void)
{
	assert (m_pNetSubSystem != 0);

	CIPAddress NTPServerIP;
	CDNSClient DNSClient (m_pNetSubSystem);
	if (!DNSClient.Resolve (m_NTPServer, &NTPServerIP))
	{
		CLogger::Get ()->Write (FromNTPDaemon, LogWarning, "Cannot resolve: %s",
					(const char *) m_NTPServer);

		return 300;
	}
	
	CNTPClient NTPClient (m_pNetSubSystem);
	unsigned nTime = NTPClient.GetTime (NTPServerIP);
	if (nTime == 0)
	{
		CLogger::Get ()->Write (FromNTPDaemon, LogWarning, "Cannot get time from %s",
					(const char *) m_NTPServer);

		return 300;
	}

	if (CTimer::Get ()->SetTime (nTime, FALSE))
	{
		CLogger::Get ()->Write (FromNTPDaemon, LogNotice, "System time updated");
	}
	else
	{
		CLogger::Get ()->Write (FromNTPDaemon, LogWarning, "Cannot update system time");
	}

	return 900;
}
