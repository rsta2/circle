//
// udpconnection.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2018  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_net_udpconnection_h
#define _circle_net_udpconnection_h

#include <circle/net/netconfig.h>
#include <circle/net/netconnection.h>
#include <circle/net/networklayer.h>
#include <circle/net/ipaddress.h>
#include <circle/net/icmphandler.h>
#include <circle/net/netqueue.h>
#include <circle/sched/synchronizationevent.h>
#include <circle/types.h>

class CUDPConnection : public CNetConnection
{
public:
	CUDPConnection (CNetConfig	*pNetConfig,
			CNetworkLayer	*pNetworkLayer,
			CIPAddress	&rForeignIP,
			u16		 nForeignPort,
			u16		 nOwnPort);
	CUDPConnection (CNetConfig	*pNetConfig,
			CNetworkLayer	*pNetworkLayer,
			u16		 nOwnPort);
	~CUDPConnection (void);

	int Connect (void);
	int Accept (CIPAddress *pForeignIP, u16 *pForeignPort);
	int Close (void);
	
	int Send (const void *pData, unsigned nLength, int nFlags);
	int Receive (void *pBuffer, int nFlags);

	int SendTo (const void *pData, unsigned nLength, int nFlags, CIPAddress	&rForeignIP, u16 nForeignPort);
	int ReceiveFrom (void *pBuffer, int nFlags, CIPAddress *pForeignIP, u16 *pForeignPort);

	int SetOptionBroadcast (boolean bAllowed);

	boolean IsConnected (void) const;
	boolean IsTerminated (void) const;
	
	void Process (void);

	// returns: -1: invalid packet, 0: not to me, 1: packet consumed
	int PacketReceived (const void *pPacket, unsigned nLength,
			    CIPAddress &rSenderIP, CIPAddress &rReceiverIP, int nProtocol);

	// returns: 0: not to me, 1: notification consumed
	int NotificationReceived (TICMPNotificationType Type,
				  CIPAddress &rSenderIP, CIPAddress &rReceiverIP,
				  u16 nSendPort, u16 nReceivePort,
				  int nProtocol);

private:
	boolean m_bOpen;
	boolean m_bActiveOpen;
	CNetQueue m_RxQueue;
	CSynchronizationEvent m_Event;
	boolean m_bBroadcastsAllowed;

	int m_nErrno;				// signalize error to the user
};

#endif
