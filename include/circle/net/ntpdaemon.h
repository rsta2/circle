//
// ntpdaemon.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_net_ntpdaemon_h
#define _circle_net_ntpdaemon_h

#include <circle/sched/task.h>
#include <circle/net/netsubsystem.h>
#include <circle/string.h>

class CNTPDaemon : public CTask
{
public:
	CNTPDaemon (const char	  *pNTPServer,		// Hostname
		    CNetSubSystem *pNetSubSystem);
	~CNTPDaemon (void);

	void Run (void);

private:
	unsigned UpdateTime (void);			// returns seconds to next update attempt

private:
	CString		 m_NTPServer;
	CNetSubSystem	*m_pNetSubSystem;
};

#endif
