//
// netconnection.h
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
#ifndef _circle_net_netconnection_h
#define _circle_net_netconnection_h

#include <circle/net/netconfig.h>
#include <circle/net/networklayer.h>
#include <circle/net/ipaddress.h>
#include <circle/net/icmphandler.h>
#include <circle/net/checksumcalculator.h>
#include <circle/types.h>

class CNetConnection
{
public:
	CNetConnection (CNetConfig	*pNetConfig,
			CNetworkLayer	*pNetworkLayer,
			CIPAddress	&rForeignIP,
			u16		 nForeignPort,
			u16		 nOwnPort,
			int		 nProtocol);
	CNetConnection (CNetConfig	*pNetConfig,
			CNetworkLayer	*pNetworkLayer,
			u16		 nOwnPort,
			int		 nProtocol);
	virtual ~CNetConnection (void);

	const u8 *GetForeignIP (void) const;
	u16 GetOwnPort (void) const;
	int GetProtocol (void) const;

	virtual int Connect (void) = 0;
	virtual int Accept (CIPAddress *pForeignIP, u16 *pForeignPort) = 0;
	virtual int Close (void) = 0;
	
	virtual int Send (const void *pData, unsigned nLength, int nFlags) = 0;
	virtual int Receive (void *pBuffer, int nFlags) = 0;

	virtual int SendTo (const void *pData, unsigned nLength, int nFlags, CIPAddress	&rForeignIP, u16 nForeignPort) = 0;
	virtual int ReceiveFrom (void *pBuffer, int nFlags, CIPAddress *pForeignIP, u16 *pForeignPort) = 0;

	virtual int SetOptionBroadcast (boolean bAllowed) = 0;

	virtual boolean IsConnected (void) const = 0;
	virtual boolean IsTerminated (void) const = 0;
	
	virtual void Process (void) = 0;

	// returns: -1: invalid packet, 0: not to me, 1: packet consumed
	virtual int PacketReceived (const void *pPacket, unsigned nLength,
				    CIPAddress &rSenderIP, CIPAddress &rReceiverIP, int nProtocol) = 0;

	// returns: 0: not to me, 1: notification consumed
	virtual int NotificationReceived (TICMPNotificationType Type,
					  CIPAddress &rSenderIP, CIPAddress &rReceiverIP,
					  u16 nSendPort, u16 nReceivePort,
					  int nProtocol) = 0;

protected:
	CNetConfig    *m_pNetConfig;
	CNetworkLayer *m_pNetworkLayer;

	CIPAddress m_ForeignIP;
	u16 m_nForeignPort;
	u16 m_nOwnPort;
	int m_nProtocol;

	CChecksumCalculator m_Checksum;
};

#endif
