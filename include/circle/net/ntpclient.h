//
// ntpclient.h
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
#ifndef _circle_net_ntpclient_h
#define _circle_net_ntpclient_h

#include <circle/net/netsubsystem.h>
#include <circle/net/ipaddress.h>

class CNTPClient
{
public:
	CNTPClient (CNetSubSystem *pNetSubSystem);
	~CNTPClient (void);

	void SetTimeZone (int nMinutesDiff);		// set minutes diff to UTC

	unsigned GetTime (CIPAddress &rServerIP);	// returns Seconds since 1970-01-01 00:00:00, 0 on error

private:
	CNetSubSystem *m_pNetSubSystem;

	int m_nMinutesDiff;
};

#endif
