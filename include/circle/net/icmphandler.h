//
// icmphandler.h
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
#ifndef _circle_net_icmphandler_h
#define _circle_net_icmphandler_h

#include <circle/net/netconfig.h>
#include <circle/net/netqueue.h>
#include <circle/net/ipaddress.h>
#include <circle/types.h>

enum TICMPNotificationType
{
	ICMPNotificationDestUnreach,
	ICMPNotificationTimeExceed,
	ICMPNotificationParamProblem,
	ICMPNotificationUnknown
};

struct TICMPNotification
{
	TICMPNotificationType	Type;
	u8			nProtocol;
	u8			SourceAddress[IP_ADDRESS_SIZE];
	u8			DestinationAddress[IP_ADDRESS_SIZE];
	u16			nSourcePort;
	u16			nDestinationPort;
};

class CNetworkLayer;
struct TIPHeader;
struct TICMPDataDatagramHeader;

class CICMPHandler
{
public:
	CICMPHandler (CNetConfig *pNetConfig, CNetworkLayer *pNetworkLayer,
		      CNetQueue *pRxQueue, CNetQueue *pNotificationQueue);
	~CICMPHandler (void);

	void Process (void);

private:
	void EnqueueNotification (TICMPNotificationType Type, TIPHeader *pIPHeader,
				  TICMPDataDatagramHeader *pDatagramHeader);

private:
	CNetConfig	*m_pNetConfig;
	CNetworkLayer	*m_pNetworkLayer;
	CNetQueue	*m_pRxQueue;
	CNetQueue	*m_pNotificationQueue;

	u8 *m_pBuffer;
};

#endif
