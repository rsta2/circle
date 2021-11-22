//
// syslogdaemon.cpp
//
// Syslog sender task according to RFC5424 and RFC5426 (UDP transport only)
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/net/syslogdaemon.h>
#include <circle/net/in.h>
#include <circle/sched/scheduler.h>
#include <circle/string.h>
#include <circle/synchronize.h>
#include <circle/util.h>
#include <assert.h>

static const char FromSysLogDaemon[] = "syslogd";

// See: RFC5424 section 6.2.1
static const struct
{
	const char *pAppName;
	unsigned    nFacility;
}
s_Sources[] =				// may has to be extended when new system sources arrive
{
	{"kernel",	    1},		// user-level messages
	{"httpd",	    3},		// system daemons
	{"tftpd",	    3},		// system daemons
	{FromSysLogDaemon,  5},		// syslogd internally
	{"ntp",		   12},		// NTP subsystem
	{"ntpd",	   12},		// NTP subsystem
	{0,		    0}		// kernel messages (default)
};

// must match TLogSeverity in include/circle/logger.h
static const unsigned SysLogSeverity[] =
{
	0,	// Emergency: system is unusable		LogPanic
	2,	// Critical: critical conditions		LogError
	4,	// Warning: warning conditions			LogWarning
	5,	// Notice: normal but significant condition	LogNotice
	7	// Debug: debug-level messages			LogDebug
};

CSysLogDaemon *CSysLogDaemon::s_pThis = 0;

CSysLogDaemon::CSysLogDaemon (CNetSubSystem *pNetSubSystem,
			      const CIPAddress &ServerIP, u16 usServerPort)
:	m_pNetSubSystem (pNetSubSystem),
	m_ServerIP (ServerIP),
	m_usServerPort (usServerPort),
	m_pTimer (CTimer::Get ()),
	m_pSocket (0)
{
	assert (s_pThis == 0);
	s_pThis = this;

	SetName (FromSysLogDaemon);
}

CSysLogDaemon::~CSysLogDaemon (void)
{
	s_pThis = 0;

	delete m_pSocket;
	m_pSocket = 0;

	m_pNetSubSystem = 0;
}

void CSysLogDaemon::Run (void)
{
	CLogger *pLogger = CLogger::Get ();
	assert (pLogger != 0);

	assert (m_pNetSubSystem != 0);
	m_pNetSubSystem->GetConfig ()->GetIPAddress ()->Format (&m_Hostname);

	assert (m_pSocket == 0);
	m_pSocket = new CSocket (m_pNetSubSystem, IPPROTO_UDP);
	assert (m_pSocket != 0);

	if (m_pSocket->Bind (SYSLOG_PORT) < 0)
	{
		pLogger->Write (FromSysLogDaemon, LogError, "Cannot bind to port %u", SYSLOG_PORT);

		return;
	}

	if (m_pSocket->Connect (m_ServerIP, m_usServerPort) < 0)
	{
		pLogger->Write (FromSysLogDaemon, LogError, "Cannot connect to server");

		return;
	}

	pLogger->RegisterEventNotificationHandler (EventNotificationHandler);
	pLogger->RegisterPanicHandler (PanicHandler);

	while (1)
	{
		m_Event.Clear ();

		TLogSeverity Severity;
		char Source[LOG_MAX_SOURCE];
		char Message[LOG_MAX_MESSAGE];
		time_t Time;
		unsigned nHundredthTime;
		int nTimeZone;
		while (pLogger->ReadEvent (&Severity, Source, Message,
					   &Time, &nHundredthTime, &nTimeZone))
		{
			if (!SendMessage (Severity, Time, nHundredthTime, nTimeZone, Source, Message))
			{
				CScheduler::Get ()->Sleep (20);
			}
		}

		m_Event.Wait ();
	}
}

boolean CSysLogDaemon::SendMessage (TLogSeverity Severity,
				    time_t FullTime, unsigned nPartialTime, int nTimeNumOffset,
				    const char *pAppName, const char *pMsg)
{
	assert (pAppName != 0);
	assert (pMsg != 0);

	CString Timestamp ("-");
	CTime Time;
	Time.Set (FullTime);
	if (Time.GetYear () > 1975)
	{
		char chTimeNumOffsetSign = '+';
		if (nTimeNumOffset < 0)
		{
			chTimeNumOffsetSign = '-';
			nTimeNumOffset = -nTimeNumOffset;
		}

		Timestamp.Format ("%04u-%02u-%02uT%02u:%02u:%02u.%02u%c%02d:%02d",
				Time.GetYear (), Time.GetMonth (), Time.GetMonthDay (),
				Time.GetHours (), Time.GetMinutes (), Time.GetSeconds (), nPartialTime,
				chTimeNumOffsetSign, nTimeNumOffset / 60, nTimeNumOffset % 60);
	}

	CString SysLogMsg;
	SysLogMsg.Format ("<%u>%u %s %s %s - - - %s",
			  CalculatePriority (pAppName, Severity), SYSLOG_VERSION,
			  (const char *) Timestamp, (const char *) m_Hostname, pAppName, pMsg);

	assert (m_pSocket != 0);
	if (   m_pSocket->Send ((const char *) SysLogMsg, SysLogMsg.GetLength (), MSG_DONTWAIT)
	    != (int) SysLogMsg.GetLength ())
	{
		return FALSE;
	}

	return TRUE;
}

unsigned CSysLogDaemon::CalculatePriority (const char *pSource, TLogSeverity Severity)
{
	assert (pSource != 0);
	assert (Severity <= LogDebug);

	unsigned nFacility = 0;
	for (unsigned i = 0; s_Sources[i].pAppName != 0; i++)
	{
		if (strcmp (pSource, s_Sources[i].pAppName) == 0)
		{
			nFacility = s_Sources[i].nFacility;
			break;
		}
	}

	return nFacility*8 + SysLogSeverity[Severity];
}

void CSysLogDaemon::EventNotificationHandler (void)
{
	s_pThis->m_Event.Set ();
}

void CSysLogDaemon::PanicHandler (void)
{
	EnableIRQs ();		// may be called on IRQ_LEVEL, where we cannot sleep

	CScheduler::Get ()->Sleep (5);
}
