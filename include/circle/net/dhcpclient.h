//
// dhcpclient.h
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
#ifndef _circle_net_dhcpclient_h
#define _circle_net_dhcpclient_h

#include <circle/sched/task.h>
#include <circle/net/netsubsystem.h>
#include <circle/net/socket.h>
#include <circle/string.h>
#include <circle/types.h>

enum TDHCPStatus
{
	DHCPStatusSuccess,
	DHCPStatusConnectError,
	DHCPStatusTimeout,
	DHCPStatusNAK,
	DHCPStatusConfigError,
	DHCPStatusConfigChanged
};

class CDHCPClient : public CTask
{
public:
	CDHCPClient (CNetSubSystem *pNetSubSystem, const char *pHostname);
	~CDHCPClient (void);

	void Run (void);

	boolean IsBound (void) const;
	
private:
	// process the states
	TDHCPStatus SelectAndRequest (void);
	TDHCPStatus RenewOrRebind (boolean bRenew, unsigned nTimeout);

	void HaltNetwork (void);

	// send DISCOVER or REQUEST and wait for reply
	boolean SendAndReceive (boolean bRequest, u32 nCIAddr = 0);

	boolean SendDiscover (void);
	boolean SendRequest (u32 nCIAddr);
	boolean SendMessage (const u8 *pOptions, unsigned nOptionsSize);

	boolean ReceiveMessage (void);
	void ScanOptions (const u8 *pOptions, unsigned nOptionsSize);
	boolean CheckConfig (void);

	u32 GetXID (void) const;

	static u32 GetUnaligned (const void *pPointer);
	static void SetUnaligned (void *pPointer, u32 nValue);

private:
	CNetSubSystem *m_pNetSubSystem;
	CNetConfig *m_pNetConfig;
	CString m_Hostname;

	CSocket m_Socket;

	boolean m_bIsBound;
	unsigned m_nBoundSince;		// starting uptime for timers

	u32 m_nOwnIPAddress;		// own IP address to be used
	u32 m_nServerIdentifier;	// IP address of the DHCP server to be used
	u32 m_nIPAddressLeaseTime;	// times to be used
	u32 m_nRenewalTimeValue;
	u32 m_nRebindingTimeValue;

	// for DHCP message header
	boolean m_bUseBroadcast;	// set BROADCAST bit in flags field
	u32 m_nXID;

	// for message to send
	u32 m_nTxCIAddr;

	// from received message
	u32 m_nRxYIAddr;		// your IP address

					// option code:
	u32 m_nRxSubnetMask;		// 1
	u32 m_nRxRouter;		// 3
	u32 m_nRxDNSServer;		// 6

	u32 m_nRxIPAddressLeaseTime;	// 51
	u8  m_nRxOptionsOverload;	// 52
	u8  m_nRxMessageType;		// 53
	u32 m_nRxServerIdentifier;	// 54
	u32 m_nRxRenewalTimeValue;	// 58
	u32 m_nRxRebindingTimeValue;	// 59

	static const unsigned s_TimeoutHZ[];
};

#endif
