//
// syslogdaemon.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_net_syslogdaemon_h
#define _circle_net_syslogdaemon_h

#include <circle/sched/task.h>
#include <circle/net/netsubsystem.h>
#include <circle/net/socket.h>
#include <circle/net/ipaddress.h>
#include <circle/sched/synchronizationevent.h>
#include <circle/logger.h>
#include <circle/timer.h>
#include <circle/time.h>
#include <circle/types.h>

#define SYSLOG_VERSION		1
#define SYSLOG_PORT		514

class CSysLogDaemon : public CTask
{
public:
	CSysLogDaemon (CNetSubSystem *pNetSubSystem,
		       const CIPAddress &ServerIP, u16 usServerPort = SYSLOG_PORT);
	~CSysLogDaemon (void);

	void Run (void);

private:
	boolean SendMessage (TLogSeverity Severity,
			     time_t FullTime, unsigned nPartialTime, int nTimeNumOffset,
			     const char *pAppName, const char *pMsg);

	unsigned CalculatePriority (const char *pSource, TLogSeverity Severity);

	static void EventNotificationHandler (void);
	static void PanicHandler (void);

private:
	CNetSubSystem *m_pNetSubSystem;
	CIPAddress m_ServerIP;
	u16 m_usServerPort;

	CTimer *m_pTimer;
	CString m_Hostname;

	CSocket *m_pSocket;

	CSynchronizationEvent m_Event;

	static CSysLogDaemon *s_pThis;
};

#endif
