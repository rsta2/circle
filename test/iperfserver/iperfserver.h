//
// iperfserver.h
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
#ifndef _iperfserver_h
#define _iperfserver_h

#include <circle/sched/task.h>
#include <circle/net/netsubsystem.h>
#include <circle/net/socket.h>
#include <circle/net/ipaddress.h>

#define IPERF_PORT	5001

#define MAX_CLIENTS	5

class CIPerfServer : public CTask		// for iperf2
{
public:
	CIPerfServer (CNetSubSystem	*pNetSubSystem,
		     CSocket		*pSocket      = 0, // is 0 for 1st created instance (listener)
		     const CIPAddress	*pClientIP    = 0,
		     u16		 usClientPort = 0);
	~CIPerfServer (void);

	void Run (void);

private:
	void Listener (void);		// accepts incoming connections and creates worker task
	void Worker (void);		// processes a connection

private:
	CNetSubSystem *m_pNetSubSystem;
	CSocket	      *m_pSocket;
	CIPAddress     m_ClientIP;
	u16	       m_usClientPort;

	static unsigned s_nInstanceCount;
};

#endif
