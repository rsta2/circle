//
// linklayer.h
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
#ifndef _circle_net_linklayer_h
#define _circle_net_linklayer_h

#include <circle/net/netconfig.h>
#include <circle/net/netdevlayer.h>
#include <circle/net/arphandler.h>
#include <circle/net/ipaddress.h>
#include <circle/usb/macaddress.h>
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

class CLinkLayer
{
public:
	CLinkLayer (CNetConfig *pNetConfig, CNetDeviceLayer *pNetDevLayer);
	~CLinkLayer (void);

	boolean Initialize (void);

	void Process (void);

	boolean Send (const CIPAddress &rReceiver, const void *pIPPacket, unsigned nLength);

	// pBuffer must have size FRAME_BUFFER_SIZE
	boolean Receive (void *pBuffer, unsigned *pResultLength);

private:
	CNetConfig *m_pNetConfig;
	CNetDeviceLayer *m_pNetDevLayer;
	CARPHandler *m_pARPHandler;

	CNetQueue m_ARPRxQueue;
	CNetQueue m_IPRxQueue;

	unsigned char *m_pBuffer;
};

#endif
