//
// networklayer.h
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
#ifndef _circle_net_networklayer_h
#define _circle_net_networklayer_h

#include <circle/net/netconfig.h>
#include <circle/net/linklayer.h>
#include <circle/net/netqueue.h>
#include <circle/net/ipaddress.h>
#include <circle/net/icmphandler.h>
#include <circle/types.h>

#define IP_OPTION_SIZE		0	// not used so far

struct TNetworkPrivateData
{
	u8	nProtocol;
	u8	SourceAddress[IP_ADDRESS_SIZE];
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
	boolean Receive (void *pBuffer, unsigned *pResultLength, CIPAddress *pSender, int *pProtocol);

private:
	CNetConfig   *m_pNetConfig;
	CLinkLayer   *m_pLinkLayer;
	CICMPHandler *m_pICMPHandler;

	CNetQueue m_RxQueue;
	CNetQueue m_ICMPRxQueue;

	u8 *m_pBuffer;
};

#endif
