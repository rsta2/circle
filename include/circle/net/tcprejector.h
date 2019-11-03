//
// tcprejector.h
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
#ifndef _circle_net_tcprejector_h
#define _circle_net_tcprejector_h

#include <circle/net/netconnection.h>
#include <circle/net/netconfig.h>
#include <circle/net/networklayer.h>
#include <circle/net/ipaddress.h>
#include <circle/net/icmphandler.h>
#include <circle/types.h>

class CTCPRejector : public CNetConnection
{
public:
	CTCPRejector (CNetConfig *pNetConfig, CNetworkLayer *pNetworkLayer);
	~CTCPRejector (void);

	// returns: -1: invalid packet, 0: not to me, 1: packet consumed
	int PacketReceived (const void *pPacket, unsigned nLength,
			    CIPAddress &rSenderIP, CIPAddress &rReceiverIP, int nProtocol);

	// unused
	int Connect (void)						{ return -1; }
	int Accept (CIPAddress *pForeignIP, u16 *pForeignPort)		{ return -1; }
	int Close (void)						{ return -1; }
	int Send (const void *pData, unsigned nLength, int nFlags)	{ return -1; }
	int Receive (void *pBuffer, int nFlags)				{ return -1; }
	int SendTo (const void *pData, unsigned nLength, int nFlags,
		    CIPAddress	&rForeignIP, u16 nForeignPort)		{ return -1; }
	int ReceiveFrom (void *pBuffer, int nFlags,
			 CIPAddress *pForeignIP, u16 *pForeignPort)	{ return -1; }
	int SetOptionBroadcast (boolean bAllowed)			{ return -1; }
	boolean IsConnected (void) const				{ return FALSE; }
	boolean IsTerminated (void) const				{ return FALSE; }
	void Process (void)						{ }
	int NotificationReceived (TICMPNotificationType Type,
				  CIPAddress &rSenderIP, CIPAddress &rReceiverIP,
				  u16 nSendPort, u16 nReceivePort,
				  int nProtocol)			{ return 0; }

private:
	boolean SendSegment (unsigned nFlags, u32 nSequenceNumber, u32 nAcknowledgmentNumber = 0);
};

#endif
