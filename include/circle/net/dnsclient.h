//
// dnsclient.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2017  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_net_dnsclient_h
#define _circle_net_dnsclient_h

#include <circle/net/netsubsystem.h>
#include <circle/net/ipaddress.h>
#include <circle/types.h>

class CDNSClient
{
public:
	CDNSClient (CNetSubSystem *pNetSubSystem);
	~CDNSClient (void);

	boolean Resolve (const char *pHostname, CIPAddress *pIPAddress);

private:
	boolean ConvertIPString (const char *pIPString, CIPAddress *pIPAddress);

private:
	CNetSubSystem *m_pNetSubSystem;

	static u16 s_nXID;		// transaction ID
};

#endif
