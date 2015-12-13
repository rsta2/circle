//
// netsubsystem.h
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
#ifndef _circle_net_netsubsystem_h
#define _circle_net_netsubsystem_h

#include <circle/net/netconfig.h>
#include <circle/net/netdevlayer.h>
#include <circle/net/linklayer.h>
#include <circle/net/networklayer.h>
#include <circle/net/transportlayer.h>
#include <circle/types.h>

class CDHCPClient;

class CNetSubSystem
{
public:
	CNetSubSystem (const u8 *pIPAddress      = 0,	// use DHCP if pIPAddress == 0
		       const u8 *pNetMask        = 0,
		       const u8 *pDefaultGateway = 0,
		       const u8 *pDNSServer      = 0);
	~CNetSubSystem (void);
	
	boolean Initialize (void);

	void Process (void);

	CNetConfig *GetConfig (void);
	CNetDeviceLayer *GetNetDeviceLayer (void);
	CTransportLayer *GetTransportLayer (void);

	boolean IsRunning (void) const;			// is DHCP bound if used?

private:
	CNetConfig	m_Config;
	CNetDeviceLayer	m_NetDevLayer;
	CLinkLayer	m_LinkLayer;
	CNetworkLayer	m_NetworkLayer;
	CTransportLayer	m_TransportLayer;

	boolean		m_bUseDHCP;
	CDHCPClient    *m_pDHCPClient;
};

#endif
