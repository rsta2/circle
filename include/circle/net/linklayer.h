//
// linklayer.h
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
#ifndef _circle_net_linklayer_h
#define _circle_net_linklayer_h

#include <circle/net/netconfig.h>
#include <circle/net/netdevlayer.h>
#include <circle/net/arphandler.h>
#include <circle/net/ipaddress.h>
#include <circle/macaddress.h>
#include <circle/net/netqueue.h>
#include <circle/macros.h>
#include <circle/types.h>

struct TEthernetHeader
{
	u8	MACReceiver[MAC_ADDRESS_SIZE];
	u8	MACSender[MAC_ADDRESS_SIZE];
	u16	nProtocolType;
#define ETH_PROT_IP		0x800
#define ETH_PROT_ARP		0x806
}
PACKED;

class CNetworkLayer;

class CLinkLayer
{
public:
	CLinkLayer (CNetConfig *pNetConfig, CNetDeviceLayer *pNetDevLayer);
	~CLinkLayer (void);

	boolean Initialize (void);

	// we need a back way to the network layer for notification
	void AttachLayer (CNetworkLayer *pNetworkLayer);

	void Process (void);

	boolean Send (const CIPAddress &rReceiver, const void *pIPPacket, unsigned nLength);

	// pBuffer must have size FRAME_BUFFER_SIZE
	boolean Receive (void *pBuffer, unsigned *pResultLength);

public:
	boolean SendRaw (const void *pFrame, unsigned nLength);

	// pBuffer must have size FRAME_BUFFER_SIZE
	boolean ReceiveRaw (void *pBuffer, unsigned *pResultLength, CMACAddress *pSender = 0);

	// nProtocolType is in host byte order
	boolean EnableReceiveRaw (u16 nProtocolType);

private:
	// return IP packet to the network layer for notification
	void ResolveFailed (const void *pReturnedFrame, unsigned nLength);
	friend class CARPHandler;

private:
	CNetConfig *m_pNetConfig;
	CNetDeviceLayer *m_pNetDevLayer;
	CNetworkLayer *m_pNetworkLayer;
	CARPHandler *m_pARPHandler;

	CNetQueue m_ARPRxQueue;
	CNetQueue m_IPRxQueue;

	CNetQueue m_RawRxQueue;
	u16 m_nRawProtocolType;
};

#endif
