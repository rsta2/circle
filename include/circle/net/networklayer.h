//
// networklayer.h
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
#ifndef _circle_net_networklayer_h
#define _circle_net_networklayer_h

#include <circle/net/netconfig.h>
#include <circle/net/linklayer.h>
#include <circle/net/netqueue.h>
#include <circle/net/ipaddress.h>
#include <circle/net/icmphandler.h>
#include <circle/net/routecache.h>
#include <circle/macros.h>
#include <circle/types.h>

struct TIPHeader
{
	u8	nVersionIHL;
#define IP_VERSION			4
#define IP_HEADER_LENGTH_DWORD_MIN	5
#define IP_HEADER_LENGTH_DWORD_MAX	6
	u8	nTypeOfService;
#define IP_TOS_ROUTINE			0
	u16	nTotalLength;
	u16	nIdentification;
#define IP_IDENTIFICATION_DEFAULT	0
	u16	nFlagsFragmentOffset;
#define IP_FRAGMENT_OFFSET(field)	((field) & 0x1F00)
	#define IP_FRAGMENT_OFFSET_FIRST	0
#define IP_FLAGS_DF			(1 << 6)	// valid without BE()
#define IP_FLAGS_MF			(1 << 5)
	u8	nTTL;
#define IP_TTL_DEFAULT			64
	u8	nProtocol;				// see: in.h
	u16	nHeaderChecksum;
	u8	SourceAddress[IP_ADDRESS_SIZE];
	u8	DestinationAddress[IP_ADDRESS_SIZE];
	//u32	nOptionsPadding;			// optional
#define IP_OPTION_SIZE		0	// not used so far
}
PACKED;

struct TNetworkPrivateData
{
	u8	nProtocol;
	u8	SourceAddress[IP_ADDRESS_SIZE];
	u8	DestinationAddress[IP_ADDRESS_SIZE];
};

class CNetworkLayer
{
public:
	CNetworkLayer (CNetConfig *pNetConfig, CLinkLayer *pLinkLayer);
	~CNetworkLayer (void);

	boolean Initialize (void);

	void Process (void);

	boolean Send (const CIPAddress &rReceiver, const void *pPacket, unsigned nLength, int nProtocol);

	// pBuffer must have size FRAME_BUFFER_SIZE
	boolean Receive (void *pBuffer, unsigned *pResultLength,
			 CIPAddress *pSender, CIPAddress *pReceiver, int *pProtocol);

	boolean ReceiveNotification (TICMPNotificationType *pType,
				     CIPAddress *pSender, CIPAddress *pReceiver,
				     u16 *pSendPort, u16 *pReceivePort,
				     int *pProtocol);

private:
	void AddRoute (const u8 *pDestIP, const u8 *pGatewayIP);
	const u8 *GetGateway (const u8 *pDestIP) const;
	friend class CICMPHandler;

	// post IP packet to the ICMP handler for notification
	void SendFailed (unsigned nICMPCode, const void *pReturnedPacket, unsigned nLength);
	friend class CLinkLayer;

private:
	CNetConfig   *m_pNetConfig;
	CLinkLayer   *m_pLinkLayer;
	CICMPHandler *m_pICMPHandler;

	CNetQueue m_RxQueue;
	CNetQueue m_ICMPRxQueue;
	CNetQueue m_ICMPNotificationQueue;

	CRouteCache m_RouteCache;
};

#endif
