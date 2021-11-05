//
// echoserver.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015  R. Stange <rsta2@o2online.de>
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
#ifndef _echoserver_h
#define _echoserver_h

#include <circle/sched/task.h>
#include <circle/net/netsubsystem.h>
#include <circle/net/socket.h>
#include <circle/net/ipaddress.h>

#define ECHO_PORT	7

#define MAX_CLIENTS	4

class CEchoServer : public CTask
{
public:
	CEchoServer (CNetSubSystem	*pNetSubSystem,
		     CSocket		*pSocket   = 0,		// is 0 for 1st created instance (listener)
		     const CIPAddress	*pClientIP = 0);
	~CEchoServer (void);

	void Run (void);

private:
	void Listener (void);			// accepts incoming connections and creates worker task
	void Worker (void);			// processes a connection

private:
	CNetSubSystem *m_pNetSubSystem;
	CSocket	      *m_pSocket;
	CIPAddress     m_ClientIP;

	static unsigned s_nInstanceCount;
};

#endif
