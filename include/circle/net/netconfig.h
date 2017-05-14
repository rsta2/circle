//
// netconfig.h
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
#ifndef _circle_net_netconfig_h
#define _circle_net_netconfig_h

#include <circle/net/ipaddress.h>
#include <circle/types.h>

class CNetConfig
{
public:
	CNetConfig (void);
	~CNetConfig (void);

	void Reset (void);

	void SetDHCP (boolean bUsed);

	void SetIPAddress (u32 nAddress);
	void SetNetMask (u32 nNetMask);
	void SetDefaultGateway (u32 nAddress);
	void SetDNSServer (u32 nAddress);

	void SetIPAddress (const u8 *pAddress);
	void SetNetMask (const u8 *pNetMask);
	void SetDefaultGateway (const u8 *pAddress);
	void SetDNSServer (const u8 *pAddress);

	boolean IsDHCPUsed (void) const;

	const CIPAddress *GetIPAddress (void) const;
	const u8 *GetNetMask (void) const;
	const CIPAddress *GetDefaultGateway (void) const;
	const CIPAddress *GetDNSServer (void) const;
	const CIPAddress *GetBroadcastAddress (void) const;		// directed broadcast

private:
	void UpdateBroadcastAddress (void);

private:
	boolean m_bUseDHCP;

	CIPAddress m_IPAddress;
	CIPAddress m_NetMask;
	CIPAddress m_DefaultGateway;
	CIPAddress m_DNSServer;
	CIPAddress m_BroadcastAddress;
};

#endif
