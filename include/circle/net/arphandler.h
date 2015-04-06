//
// arphandler.h
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
#ifndef _circle_net_arphandler_h
#define _circle_net_arphandler_h

#include <circle/net/netconfig.h>
#include <circle/net/netdevlayer.h>
#include <circle/net/netqueue.h>
#include <circle/net/ipaddress.h>
#include <circle/usb/macaddress.h>
#include <circle/spinlock.h>
#include <circle/types.h>

#define ARP_MAX_ENTRIES		20

enum TARPState
{
	ARPStateFreeSlot = 0,
	ARPStateRequestSent,
	ARPStateValid,
	ARPStateUnknown
};

struct TARPEntry
{
	volatile TARPState	State;
	u8			IPAddress[IP_ADDRESS_SIZE];
	u8			MACAddress[MAC_ADDRESS_SIZE];
	unsigned		hTimer;
	unsigned		nTicksLastUsed;
};

class CARPHandler
{
public:
	CARPHandler (CNetConfig *pNetConfig, CNetDeviceLayer *pNetDevLayer, CNetQueue *pRxQueue);
	~CARPHandler (void);

	void Process (void);

	boolean Resolve (const CIPAddress &rIPAddress, CMACAddress *pMACAddress);
	
private:
	void ReplyReceived (const CIPAddress &rForeignIP, const CMACAddress &rForeignMAC);
	void RequestReceived (const CIPAddress &rForeignIP, const CMACAddress &rForeignMAC);

	void SendPacket (boolean bRequest, const CIPAddress &rForeignIP, const CMACAddress &rForeignMAC);

	static void TimerHandler (unsigned hTimer, void *pParam, void *pContext);

private:
	CNetConfig	*m_pNetConfig;
	CNetDeviceLayer	*m_pNetDevLayer;
	CNetQueue	*m_pRxQueue;

	TARPEntry m_Entry[ARP_MAX_ENTRIES];
	CSpinLock m_SpinLock;

	u8 *m_pBuffer;
};

#endif
