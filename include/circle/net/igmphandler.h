//
// igmphandler.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2025  R. Stange <rsta2@gmx.net>
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
#ifndef _circle_net_igmphandler_h
#define _circle_net_igmphandler_h

#include <circle/net/netconfig.h>
#include <circle/net/linklayer.h>
#include <circle/net/netqueue.h>
#include <circle/net/ipaddress.h>
#include <circle/timer.h>
#include <circle/types.h>

class CNetworkLayer;

class CIGMPHandler	/// IGMP version 2 protocol handler (RFC 2236)
{
public:
	static const unsigned UnsolicitedReportInterval	= 10*HZ;
	static const unsigned V1RouterPresentTimeout	= 400*HZ;

public:
	CIGMPHandler (CNetConfig *pNetConfig, CNetworkLayer *pNetworkLayer, CLinkLayer *pLinkLayer,
		      CNetQueue *pRxQueue);
	~CIGMPHandler (void);

	void Process (void);

	boolean JoinHostGroup (const CIPAddress &rGroupAddress);
	boolean LeaveHostGroup (const CIPAddress &rGroupAddress);

private:
	// sends Membership Report or Leave Group for rGroupAddress
	boolean SendPacket (boolean bMembershipReport, const CIPAddress &rGroupAddress);

	unsigned GetRandom (unsigned nMax);

private:
	CNetConfig	*m_pNetConfig;
	CNetworkLayer	*m_pNetworkLayer;
	CLinkLayer	*m_pLinkLayer;
	CNetQueue	*m_pRxQueue;

	boolean m_bAllSystemsJoined;
	unsigned m_nLastTicks;
	unsigned m_nV1RouterTimer;			// 0 if not heard

	static const unsigned MaxHostGroups = MAX_MULTICAST_GROUPS;
	CIPAddress m_HostGroup[MaxHostGroups];
	unsigned m_nUseCounter[MaxHostGroups];
	unsigned m_nDelayTimer[MaxHostGroups];		// 0 if not running
	boolean m_bLastHostFlag[MaxHostGroups];

	unsigned m_nRandomSeed;
};

#endif
