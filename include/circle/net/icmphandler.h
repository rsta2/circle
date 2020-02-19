//
// icmphandler.h
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
#ifndef _circle_net_icmphandler_h
#define _circle_net_icmphandler_h

#include <circle/net/netconfig.h>
#include <circle/net/netqueue.h>
#include <circle/net/ipaddress.h>
#include <circle/types.h>

#define ICMP_TYPE_ECHO_REPLY	0
#define ICMP_TYPE_ECHO		8
	#define ICMP_CODE_ECHO			0
#define ICMP_TYPE_DEST_UNREACH	3
	#define ICMP_CODE_DEST_NET_UNREACH	0
	#define ICMP_CODE_DEST_HOST_UNREACH	1
	#define ICMP_CODE_DEST_PROTO_UNREACH	2
	#define ICMP_CODE_DEST_PORT_UNREACH	3
	#define ICMP_CODE_FRAG_REQUIRED		4
	#define ICMP_CODE_SRC_ROUTE_FAIL	5
	#define ICMP_CODE_DEST_NET_UNKNOWN	6
	#define ICMP_CODE_DEST_HOST_UNKNOWN	7
	#define ICMP_CODE_SRC_HOST_ISOLATED	8
	#define ICMP_CODE_NET_ADMIN_PROHIB	9
	#define ICMP_CODE_HOST_ADMIN_PROHIB	0
	#define ICMP_CODE_NET_UNREACH_TOS	11
	#define ICMP_CODE_HOST_UNREACH_TOS	12
	#define ICMP_CODE_COMM_ADMIN_PROHIB	13
	#define ICMP_CODE_HOST_PREC_VIOLAT	14
	#define ICMP_CODE_PREC_CUTOFF_EFFECT	15
#define ICMP_TYPE_REDIRECT	5
	#define ICMP_CODE_REDIRECT_NET		0
	#define ICMP_CODE_REDIRECT_HOST		1
	#define ICMP_CODE_REDIRECT_TOS_NET	2
	#define ICMP_CODE_REDIRECT_TOS_HOST	3
#define ICMP_TYPE_TIME_EXCEED	11
	#define ICMP_CODE_TTL_EXPIRED		0
	#define ICMP_CODE_FRAG_REASS_TIME_EXCEED 1
#define ICMP_TYPE_PARAM_PROBLEM	12
	#define ICMP_CODE_POINTER		0
	#define ICMP_CODE_MISSING_OPTION	1
	#define ICMP_CODE_BAD_LENGTH		2

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

	void DestinationUnreachable (unsigned nCode,
				     const void *pReturnedIPPacket, unsigned nLength);

private:
	void EnqueueNotification (TICMPNotificationType Type, TIPHeader *pIPHeader,
				  TICMPDataDatagramHeader *pDatagramHeader);

private:
	CNetConfig	*m_pNetConfig;
	CNetworkLayer	*m_pNetworkLayer;
	CNetQueue	*m_pRxQueue;
	CNetQueue	*m_pNotificationQueue;
};

#endif
