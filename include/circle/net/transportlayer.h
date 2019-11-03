//
// transportlayer.h
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
#ifndef _circle_net_transportlayer_h
#define _circle_net_transportlayer_h

#include <circle/net/netconfig.h>
#include <circle/net/networklayer.h>
#include <circle/net/netconnection.h>
#include <circle/net/tcprejector.h>
#include <circle/net/ipaddress.h>
#include <circle/net/netqueue.h>
#include <circle/ptrarray.h>
#include <circle/spinlock.h>
#include <circle/types.h>

class CTransportLayer
{
public:
	CTransportLayer (CNetConfig *pNetConfig, CNetworkLayer *pNetworkLayer);
	~CTransportLayer (void);

	boolean Initialize (void);

	void Process (void);

	// nOwnPort must not be 0
	int Bind (u16 nOwnPort, int nProtocol);

	// nOwnPort may be 0 (dynamic port assignment)
	int Connect (CIPAddress &rIPAddress, u16 nPort, u16 nOwnPort, int nProtocol);

	int Listen (u16 nOwnPort, int nProtocol);
	int Accept (CIPAddress *pForeignIP, u16 *pForeignPort, int hConnection);

	int Disconnect (int hConnection);

	int Send (const void *pData, unsigned nLength, int nFlags, int hConnection);

	// pBuffer must have size FRAME_BUFFER_SIZE
	int Receive (void *pBuffer, int nFlags, int hConnection);

	int SendTo (const void *pData, unsigned nLength, int nFlags,
		    CIPAddress &rForeignIP, u16 nForeignPort, int hConnection);

	// pBuffer must have size FRAME_BUFFER_SIZE
	int ReceiveFrom (void *pBuffer, int nFlags, CIPAddress *pForeignIP,
			 u16 *pForeignPort, int hConnection);

	int SetOptionBroadcast (boolean bAllowed, int hConnection);

	boolean IsConnected (int hConnection) const;
	const u8 *GetForeignIP (int hConnection) const;		// returns 0 if not connected

private:
	CNetConfig    *m_pNetConfig;
	CNetworkLayer *m_pNetworkLayer;

	CPtrArray m_pConnection;
	u16 m_nOwnPort;
	CSpinLock m_SpinLock;

	CTCPRejector m_TCPRejector;
};

#endif
