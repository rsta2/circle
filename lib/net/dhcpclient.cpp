//
// dhcpclient.cpp
//
// This implements a DHCP client (RFC 2131 and RFC 2132).
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/net/dhcpclient.h>
#include <circle/net/in.h>
#include <circle/macaddress.h>
#include <circle/sched/scheduler.h>
#include <circle/logger.h>
#include <circle/timer.h>
#include <circle/util.h>
#include <circle/string.h>
#include <circle/macros.h>
#include <assert.h>

#define RESTART_DELAY		60		// seconds

#define DHCP_PORT_SERVER	67
#define DHCP_PORT_CLIENT	68

struct TDHCPMessage
{
	u8	OP;
#define DHCP_OP_BOOTREQUEST	1
#define DHCP_OP_BOOTREPLY	2
	u8	HType;
#define DHCP_HTYPE_ETHER	1
	u8	HLen;				// Hardware address length
	u8	Hops;				// set to 0
	u32	XID;				// Transaction ID
	u16	Secs;				// set to 0
	u16	Flags;
#define DHCP_FLAGS_UNICAST	0
#define DHCP_FLAGS_BROADCAST	BE(0x8000)
	u32	CIAddr;				// Client IP address
	u32	YIAddr;				// Your IP address
	u32	SIAddr;				// Server IP address
	u32	GIAddr;				// Gateway IP address
	u8	CHAddr[16];			// Client hardware address (padded with 0)
	u8	SName[64];			// all bytes set to 0
	u8	File[128];			// all bytes set to 0
	u32	MagicCookie;
#define DHCP_MAGIC_COOKIE	0x63538263
	u8	Options[0];			// options follow
}
PACKED;

#define DHCP_HEADER_SIZE	(sizeof (TDHCPMessage))
#define DHCP_MAX_OPTIONS_SIZE	(312-4)		// MagicCookie is counted as part of Options but not here
#define DHCP_MAX_MESSAGE_SIZE	(DHCP_HEADER_SIZE + DHCP_MAX_OPTIONS_SIZE)

struct TDHCPOption
{
	u8	Code;
#define DHCP_OPTION_PAD		0
#define DHCP_OPTION_SUBNETMASK	1
#define DHCP_OPTION_ROUTER	3
#define DHCP_OPTION_DNSSERVER	6
#define DHCP_OPTION_HOSTNAME	12
#define DHCP_OPTION_REQIPADDR	50
#define DHCP_OPTION_LEASETIME	51
	#define DHCP_OPTION_LEASETIME_INFINITE	((u32) -1)
#define DHCP_OPTION_OVERLOAD	52
	#define DHCP_OPTION_OVERLOAD_FILE	1
	#define DHCP_OPTION_OVERLOAD_SNAME	2
	#define DHCP_OPTION_OVERLOAD_BOTH	3
#define DHCP_OPTION_MSGTYPE	53
	#define DHCP_OPTION_MSGTYPE_DISCOVER	1
	#define DHCP_OPTION_MSGTYPE_OFFER	2
	#define DHCP_OPTION_MSGTYPE_REQUEST	3
	#define DHCP_OPTION_MSGTYPE_DECLINE	4
	#define DHCP_OPTION_MSGTYPE_ACK		5
	#define DHCP_OPTION_MSGTYPE_NAK		6
	#define DHCP_OPTION_MSGTYPE_RELEASE	7
	#define DHCP_OPTION_MSGTYPE_INFORM	8
#define DHCP_OPTION_SERVERID	54
#define DHCP_OPTION_PARMLIST	55
#define DHCP_OPTION_RENEWALTIME 58
#define DHCP_OPTION_REBINDTIME	59
#define DHCP_OPTION_END		255
	u8	Len;
	u8	Value[0];
}
PACKED;

static const char FromDHCPClient[] = "dhcp";

#define MAX_TRIES	4
const unsigned CDHCPClient::s_TimeoutHZ[MAX_TRIES] = {4*HZ, 8*HZ, 16*HZ, 32*HZ};

CDHCPClient::CDHCPClient (CNetSubSystem *pNetSubSystem, const char *pHostname)
:	m_pNetSubSystem (pNetSubSystem),
	m_pNetConfig (pNetSubSystem->GetConfig ()),
	m_Hostname (pHostname != 0 ? pHostname : ""),
	m_Socket (pNetSubSystem, IPPROTO_UDP),
	m_bIsBound (FALSE)
{
	assert (m_pNetSubSystem != 0);
	assert (m_pNetConfig != 0);

	assert (m_Hostname.GetLength () <= 30);

	SetName (FromDHCPClient);
}

CDHCPClient::~CDHCPClient (void)
{
	m_pNetSubSystem = 0;
}

void CDHCPClient::Run (void)
{
	if (m_Socket.Bind (DHCP_PORT_CLIENT) < 0)
	{
		CLogger::Get ()->Write (FromDHCPClient, LogError, "Cannot bind to port %u", DHCP_PORT_CLIENT);

		return;
	}

	while (1)
	{
	InitState:
		switch (SelectAndRequest ())
		{
		case DHCPStatusSuccess:
			break;

		case DHCPStatusConnectError:
			return;			// cannot recover

		case DHCPStatusTimeout:
		case DHCPStatusNAK:
		case DHCPStatusConfigError:
		case DHCPStatusConfigChanged:
			CScheduler::Get ()->Sleep (RESTART_DELAY);
			goto InitState;
		}
	
	BoundState:
		m_nBoundSince = CTimer::Get ()->GetUptime ();

		while (CTimer::Get ()->GetUptime () - m_nBoundSince < m_nRenewalTimeValue)
		{
			// discard messages
			u8 Buffer[DHCP_MAX_MESSAGE_SIZE];
			while (m_Socket.Receive (Buffer, sizeof Buffer, MSG_DONTWAIT) > 0)
			{
				CScheduler::Get ()->Yield ();
			}

			CScheduler::Get ()->Sleep (10);
		}

	//RenewingState:
		switch (RenewOrRebind (TRUE, m_nRebindingTimeValue))
		{
		case DHCPStatusSuccess:
			goto BoundState;

		case DHCPStatusConnectError:
			return;			// cannot recover

		case DHCPStatusTimeout:
			break;
			
		case DHCPStatusNAK:
		case DHCPStatusConfigError:
		case DHCPStatusConfigChanged:
			HaltNetwork ();
			goto InitState;
		}

	//RebindingState:
		switch (RenewOrRebind (FALSE, m_nIPAddressLeaseTime))
		{
		case DHCPStatusSuccess:
			goto BoundState;

		case DHCPStatusConnectError:
			return;			// cannot recover

		case DHCPStatusTimeout:
			break;
			
		case DHCPStatusNAK:
		case DHCPStatusConfigError:
		case DHCPStatusConfigChanged:
			HaltNetwork ();
			goto InitState;
		}

		CLogger::Get ()->Write (FromDHCPClient, LogError, "Lease expired");

		HaltNetwork ();

		goto InitState;
	}
}

boolean CDHCPClient::IsBound (void) const
{
	return m_bIsBound;
}

TDHCPStatus CDHCPClient::SelectAndRequest (void)
{
	m_bUseBroadcast = TRUE;

	CIPAddress BroadcastIP;
	BroadcastIP.SetBroadcast ();
	if (m_Socket.Connect (BroadcastIP, DHCP_PORT_SERVER) < 0)
	{
		CLogger::Get ()->Write (FromDHCPClient, LogError, "Cannot connect (broadcast, port %u)", DHCP_PORT_SERVER);

		return DHCPStatusConnectError;
	}

	if (m_Socket.SetOptionBroadcast (TRUE))
	{
		CLogger::Get ()->Write (FromDHCPClient, LogError, "Cannot set broadcast option");

		return DHCPStatusConnectError;
	}

	m_nXID = GetXID ();		// new transaction ID

	// SELECTING state
	if (!SendAndReceive (FALSE))
	{
		return DHCPStatusTimeout;
	}

	m_nOwnIPAddress = m_nRxYIAddr;
	assert (m_nOwnIPAddress != 0);

	m_nServerIdentifier = m_nRxServerIdentifier;
	assert (m_nServerIdentifier != 0);

	// REQUESTING state
	if (!SendAndReceive (TRUE, 0))
	{
		return DHCPStatusTimeout;
	}

	if (m_nRxMessageType == DHCP_OPTION_MSGTYPE_NAK)
	{
		CLogger::Get ()->Write (FromDHCPClient, LogWarning, "REQUEST rejected with NAK");

		return DHCPStatusNAK;
	}

	if (!CheckConfig ())
	{
		return DHCPStatusConfigError;
	}

	if (m_nOwnIPAddress != m_nRxYIAddr)
	{
		CLogger::Get ()->Write (FromDHCPClient, LogWarning, "IP address has changed");

		return DHCPStatusConfigChanged;
	}

	if (m_nServerIdentifier != m_nRxServerIdentifier)
	{
		CLogger::Get ()->Write (FromDHCPClient, LogWarning, "Server identifier has changed");

		return DHCPStatusConfigChanged;
	}

	// TODO: send ARP request for assigned IP address and send DECLINE if it is answered

	CIPAddress IPAddress;
	IPAddress.Set (m_nOwnIPAddress);
	CString IPString;
	IPAddress.Format (&IPString);
	CLogger::Get ()->Write (FromDHCPClient, LogNotice, "IP address is %s", (const char *) IPString);

	assert (m_pNetConfig != 0);
	m_pNetConfig->SetIPAddress (m_nOwnIPAddress);
	m_pNetConfig->SetNetMask (m_nRxSubnetMask);
	m_pNetConfig->SetDefaultGateway (m_nRxRouter);
	m_pNetConfig->SetDNSServer (m_nRxDNSServer);

	m_nIPAddressLeaseTime = m_nRxIPAddressLeaseTime;
	m_nRenewalTimeValue   = m_nRxRenewalTimeValue;
	m_nRebindingTimeValue = m_nRxRebindingTimeValue;

	m_bIsBound = TRUE;

	// TODO: send ARP reply to inform other clients

	return DHCPStatusSuccess;
}

TDHCPStatus CDHCPClient::RenewOrRebind (boolean bRenew, unsigned nTimeout)
{
	if (CTimer::Get ()->GetUptime () - m_nBoundSince >= nTimeout)
	{
		return DHCPStatusTimeout;
	}

	CLogger::Get ()->Write (FromDHCPClient, LogDebug, "%s lease", bRenew ? "Renewing" : "Rebinding");

	CIPAddress DestIP;

	if (bRenew)
	{
		m_bUseBroadcast = FALSE;

		DestIP.Set (m_nServerIdentifier);
	}
	else
	{
		m_bUseBroadcast = TRUE;

		DestIP.SetBroadcast ();
	}

	if (m_Socket.Connect (DestIP, DHCP_PORT_SERVER) < 0)
	{
		CLogger::Get ()->Write (FromDHCPClient, LogError, "Cannot connect (%scast, port %u)", 
					m_bUseBroadcast ? "broad" : "uni", DHCP_PORT_SERVER);

		return DHCPStatusConnectError;
	}

	if (m_Socket.SetOptionBroadcast (m_bUseBroadcast))
	{
		CLogger::Get ()->Write (FromDHCPClient, LogError, "Cannot set broadcast option");

		return DHCPStatusConnectError;
	}

	boolean bReceiveOK = FALSE;
	while (CTimer::Get ()->GetUptime () - m_nBoundSince < nTimeout)
	{
		m_nXID = GetXID ();		// new transaction ID

		if (SendAndReceive (TRUE, m_nOwnIPAddress))
		{
			bReceiveOK = TRUE;

			break;
		}

		unsigned nTimeElapsed = CTimer::Get ()->GetUptime () - m_nBoundSince;
		if (nTimeout <= nTimeElapsed)
		{
			return DHCPStatusTimeout;
		}

		unsigned nTimeLeft = nTimeout - nTimeElapsed;

		// see RFC 2131 4.4.5 8th paragraph
		unsigned nSleepTime = nTimeLeft / 2;
		if (nSleepTime < 60)
		{
			nSleepTime = 60;
		}
		
		CScheduler::Get ()->Sleep (nSleepTime);
	}

	if (!bReceiveOK)
	{
		return DHCPStatusTimeout;
	}
	
	if (m_nRxMessageType == DHCP_OPTION_MSGTYPE_NAK)
	{
		CLogger::Get ()->Write (FromDHCPClient, LogWarning, "REQUEST rejected with NAK");

		return DHCPStatusNAK;
	}

	if (!CheckConfig ())
	{
		return DHCPStatusConfigError;
	}

	m_nServerIdentifier = m_nRxServerIdentifier;
	if (m_nServerIdentifier == 0)
	{
		CLogger::Get ()->Write (FromDHCPClient, LogWarning, "Server identifier is not set");

		return DHCPStatusConfigError;
	}

	assert (m_pNetConfig != 0);
	if (   *m_pNetConfig->GetIPAddress ()       != m_nRxYIAddr
	    && *(u32 *) m_pNetConfig->GetNetMask () != m_nRxSubnetMask
	    && *m_pNetConfig->GetDefaultGateway ()  != m_nRxRouter
	    && *m_pNetConfig->GetDNSServer ()       != m_nRxDNSServer)
	{
		CLogger::Get ()->Write (FromDHCPClient, LogWarning, "Network configuration has changed");

		return DHCPStatusConfigChanged;
	}

	// times may have changed
	m_nIPAddressLeaseTime = m_nRxIPAddressLeaseTime;
	m_nRenewalTimeValue   = m_nRxRenewalTimeValue;
	m_nRebindingTimeValue = m_nRxRebindingTimeValue;

	CLogger::Get ()->Write (FromDHCPClient, LogDebug, "%s completed", bRenew ? "Renewing" : "Rebinding");

	return DHCPStatusSuccess;
}

void CDHCPClient::HaltNetwork (void)
{
	m_bIsBound = FALSE;

	assert (m_pNetConfig != 0);
	m_pNetConfig->Reset ();
}

boolean CDHCPClient::SendAndReceive (boolean bRequest, u32 nCIAddr)
{
	for (unsigned nTry = 1; nTry <= MAX_TRIES; nTry++)
	{
		if (!(bRequest ? SendRequest (nCIAddr) : SendDiscover ()))
		{
			CLogger::Get ()->Write (FromDHCPClient, LogError, "Cannot send %s",
						bRequest ? "REQUEST" : "DISCOVER");

			return FALSE;
		}

		unsigned nStartTicks = CTimer::Get ()->GetTicks ();	// use ticks here for precision
		while (CTimer::Get ()->GetTicks () - nStartTicks < s_TimeoutHZ[nTry-1])
		{
			if (ReceiveMessage ())
			{
				if (bRequest)
				{
					if (   m_nRxMessageType == DHCP_OPTION_MSGTYPE_ACK
					    || m_nRxMessageType == DHCP_OPTION_MSGTYPE_NAK)
					{
						return TRUE;
					}
				}
				else
				{
					// take the first received OFFER if useable
					if (   m_nRxMessageType == DHCP_OPTION_MSGTYPE_OFFER
					    && CheckConfig ()
					    && m_nRxServerIdentifier != 0)
					{
						return TRUE;
					}
				}
			}

			CScheduler::Get ()->Yield ();
		}

		CLogger::Get ()->Write (FromDHCPClient, LogWarning, "No response from server. Retrying.");
	}

	CLogger::Get ()->Write (FromDHCPClient, LogWarning, "Did not receive %s",
				bRequest ? "ACK or NAK" : "OFFER");

	return FALSE;
}

boolean CDHCPClient::SendDiscover (void)
{
	m_nTxCIAddr = 0;

	static const u8 Options[] =
	{
		DHCP_OPTION_MSGTYPE,  1, DHCP_OPTION_MSGTYPE_DISCOVER,
		DHCP_OPTION_PARMLIST, 6,
			DHCP_OPTION_SUBNETMASK,
			DHCP_OPTION_ROUTER,
			DHCP_OPTION_DNSSERVER,
			DHCP_OPTION_LEASETIME,
			DHCP_OPTION_RENEWALTIME,
			DHCP_OPTION_REBINDTIME,
		DHCP_OPTION_END
	};

	return SendMessage (Options, sizeof Options);
}

boolean CDHCPClient::SendRequest (u32 nCIAddr)
{
	m_nTxCIAddr = nCIAddr;

	const u8 *pOptions;
	unsigned nOptionsSize;

	if (nCIAddr == 0)
	{
		static u8 Options[] =
		{
			DHCP_OPTION_MSGTYPE,   1, DHCP_OPTION_MSGTYPE_REQUEST,
			DHCP_OPTION_SERVERID,  4, 0, 0, 0, 0,
#define REQ_OFFSET_SERVERID	5
			DHCP_OPTION_REQIPADDR, 4, 0, 0, 0, 0,
#define REQ_OFFSET_REQIPADDR	11
			DHCP_OPTION_PARMLIST,  6,
				DHCP_OPTION_SUBNETMASK,
				DHCP_OPTION_ROUTER,
				DHCP_OPTION_DNSSERVER,
				DHCP_OPTION_LEASETIME,
				DHCP_OPTION_RENEWALTIME,
				DHCP_OPTION_REBINDTIME,
			DHCP_OPTION_END
		};

		SetUnaligned (Options+REQ_OFFSET_SERVERID,  m_nServerIdentifier);
		SetUnaligned (Options+REQ_OFFSET_REQIPADDR, m_nOwnIPAddress);

		pOptions = Options;
		nOptionsSize = sizeof Options;
	}
	else
	{
		const static u8 Options[] =
		{
			DHCP_OPTION_MSGTYPE,   1, DHCP_OPTION_MSGTYPE_REQUEST,
			DHCP_OPTION_PARMLIST,  6,
				DHCP_OPTION_SUBNETMASK,
				DHCP_OPTION_ROUTER,
				DHCP_OPTION_DNSSERVER,
				DHCP_OPTION_LEASETIME,
				DHCP_OPTION_RENEWALTIME,
				DHCP_OPTION_REBINDTIME,
			DHCP_OPTION_END
		};

		pOptions = Options;
		nOptionsSize = sizeof Options;
	}

	unsigned nHostnameLen = m_Hostname.GetLength ();
	if (nHostnameLen == 0)
	{
		return SendMessage (pOptions, nOptionsSize);
	}

	// insert host name option
	assert (nHostnameLen <= 255);
	unsigned nOptionsBufferSize = nOptionsSize + nHostnameLen+2;
	u8 OptionsBuffer[nOptionsBufferSize];
	u8 *p = OptionsBuffer;

	memcpy (p, pOptions, nOptionsSize);
	p += nOptionsSize-1;			// ignore end option

	*p++ = DHCP_OPTION_HOSTNAME;
	*p++ = (u8) nHostnameLen;
	memcpy (p, (const char *) m_Hostname, nHostnameLen);
	p += nHostnameLen;

	*p = DHCP_OPTION_END;

	return SendMessage (OptionsBuffer, nOptionsBufferSize);
}

boolean CDHCPClient::SendMessage (const u8 *pOptions, unsigned nOptionsSize)
{
	u8 Buffer[DHCP_MAX_MESSAGE_SIZE];
	memset (Buffer, 0, sizeof Buffer);

	TDHCPMessage *pMessage = (TDHCPMessage *) Buffer;

	// need not set fields to 0 because of memset before
	pMessage->OP		= DHCP_OP_BOOTREQUEST;
	pMessage->HType		= DHCP_HTYPE_ETHER;
	pMessage->HLen		= MAC_ADDRESS_SIZE;
	//pMessage->Hops	= 0;
	pMessage->XID		= m_nXID;
	//pMessage->Secs	= 0;
	pMessage->Flags		= m_bUseBroadcast ? DHCP_FLAGS_BROADCAST : DHCP_FLAGS_UNICAST;
	pMessage->CIAddr	= m_nTxCIAddr;
	//pMessage->YIAddr	= 0;
	//pMessage->SIAddr	= 0;
	//pMessage->GIAddr	= 0;
	pMessage->MagicCookie	= DHCP_MAGIC_COOKIE;

	assert (m_pNetSubSystem != 0);
	const CMACAddress *pMACAddress = m_pNetSubSystem->GetNetDeviceLayer ()->GetMACAddress ();
	assert (pMACAddress != 0);
	pMACAddress->CopyTo (pMessage->CHAddr);

	assert (pOptions != 0);
	assert (nOptionsSize <= DHCP_MAX_OPTIONS_SIZE);
	memcpy (pMessage->Options, pOptions, nOptionsSize);

	// we always send a full sized message (with padding)
	return m_Socket.Send (pMessage, DHCP_MAX_MESSAGE_SIZE, 0) == DHCP_MAX_MESSAGE_SIZE;
}

boolean CDHCPClient::ReceiveMessage (void)
{
	u8 Buffer[DHCP_MAX_MESSAGE_SIZE];

	int nMessageSize = m_Socket.Receive (Buffer, sizeof Buffer, MSG_DONTWAIT);
	if (nMessageSize < (int) DHCP_HEADER_SIZE)
	{
		return FALSE;
	}

	TDHCPMessage *pMessage = (TDHCPMessage *) Buffer;

	// check message
	if (   pMessage->OP	     != DHCP_OP_BOOTREPLY
	    || pMessage->HType	     != DHCP_HTYPE_ETHER
	    || pMessage->HLen	     != MAC_ADDRESS_SIZE
	    || pMessage->XID	     != m_nXID
	    || pMessage->MagicCookie != DHCP_MAGIC_COOKIE)
	{
		return FALSE;
	}

	assert (m_pNetSubSystem != 0);
	const CMACAddress *pOwnMACAddress = m_pNetSubSystem->GetNetDeviceLayer ()->GetMACAddress ();
	assert (pOwnMACAddress != 0);

	CMACAddress CHAddr (pMessage->CHAddr);
	if (CHAddr != *pOwnMACAddress)
	{
		return FALSE;
	}

	// all other header fields are not interesting for us
	m_nRxYIAddr = pMessage->YIAddr;

	// 0 is an invalid value for all these options:
	m_nRxSubnetMask		= 0;
	m_nRxRouter		= 0;
	m_nRxDNSServer		= 0;
	m_nRxIPAddressLeaseTime = 0;
	m_nRxOptionsOverload	= 0;
	m_nRxMessageType	= 0;
	m_nRxServerIdentifier	= 0;
	m_nRxRenewalTimeValue	= 0;
	m_nRxRebindingTimeValue	= 0;

	ScanOptions (pMessage->Options, nMessageSize-DHCP_HEADER_SIZE);

	switch (m_nRxOptionsOverload)
	{
	case DHCP_OPTION_OVERLOAD_FILE:
		ScanOptions (pMessage->File, sizeof pMessage->File);
		break;

	case DHCP_OPTION_OVERLOAD_SNAME:
		ScanOptions (pMessage->SName, sizeof pMessage->SName);
		break;

	case DHCP_OPTION_OVERLOAD_BOTH:
		ScanOptions (pMessage->File, sizeof pMessage->File);
		ScanOptions (pMessage->SName, sizeof pMessage->SName);
		break;

	default:
		break;
	}

	return TRUE;
}

void CDHCPClient::ScanOptions (const u8 *pOptions, unsigned nOptionsSize)
{
	assert (pOptions != 0);
	u8 *pOptionsEnd = (u8 *) pOptions+nOptionsSize;

	TDHCPOption *pOption = (TDHCPOption *) pOptions;
	while ((u8 *) pOption+2 <= pOptionsEnd)
	{
		switch (pOption->Code)
		{
		case DHCP_OPTION_END:
			return;

		case DHCP_OPTION_PAD:
			pOption = (TDHCPOption *) ((u8 *) pOption+1);
			break;
			
		case DHCP_OPTION_SUBNETMASK:
			if (   pOption->Len == 4
			    && (u8 *) pOption+4+2 <= pOptionsEnd)
			{
				m_nRxSubnetMask = GetUnaligned (pOption->Value);
			}
			goto Skip;

		case DHCP_OPTION_ROUTER:
			if (   pOption->Len >= 4
			    && (u8 *) pOption+4+2 <= pOptionsEnd)
			{
				if (m_nRxRouter == 0)		// may be present more than once (?)
				{
					m_nRxRouter = GetUnaligned (pOption->Value); // take the 1st router
				}
			}
			goto Skip;

		case DHCP_OPTION_DNSSERVER:
			if (   pOption->Len >= 4
			    && (u8 *) pOption+4+2 <= pOptionsEnd)
			{
				m_nRxDNSServer = GetUnaligned (pOption->Value);	// take the 1st DNS server
			}
			goto Skip;

		case DHCP_OPTION_LEASETIME:
			if (   pOption->Len == 4
			    && (u8 *) pOption+4+2 <= pOptionsEnd)
			{
				m_nRxIPAddressLeaseTime = be2le32 (GetUnaligned (pOption->Value));
			}
			goto Skip;

		case DHCP_OPTION_OVERLOAD:
			if (   pOption->Len == 1
			    && (u8 *) pOption+1+2 <= pOptionsEnd)
			{
				m_nRxOptionsOverload = pOption->Value[0];
			}
			goto Skip;

		case DHCP_OPTION_MSGTYPE:
			if (   pOption->Len == 1
			    && (u8 *) pOption+1+2 <= pOptionsEnd)
			{
				m_nRxMessageType = pOption->Value[0];
			}
			goto Skip;

		case DHCP_OPTION_SERVERID:
			if (   pOption->Len == 4
			    && (u8 *) pOption+4+2 <= pOptionsEnd)
			{
				m_nRxServerIdentifier = GetUnaligned (pOption->Value);
			}
			goto Skip;

		case DHCP_OPTION_RENEWALTIME:
			if (   pOption->Len == 4
			    && (u8 *) pOption+4+2 <= pOptionsEnd)
			{
				m_nRxRenewalTimeValue = be2le32 (GetUnaligned (pOption->Value));
			}
			goto Skip;

		case DHCP_OPTION_REBINDTIME:
			if (   pOption->Len == 4
			    && (u8 *) pOption+4+2 <= pOptionsEnd)
			{
				m_nRxRebindingTimeValue = be2le32 (GetUnaligned (pOption->Value));
			}
			goto Skip;

		Skip:
		default:
			pOption = (TDHCPOption *) ((u8 *) pOption+pOption->Len+2);
			break;
		}
	}
}

boolean CDHCPClient::CheckConfig (void)
{
	// all addresses and netmask must be set
	if (   m_nRxYIAddr     == 0
	    || m_nRxSubnetMask == 0
	    || m_nRxRouter     == 0
	    || m_nRxDNSServer  == 0)
	{
		CLogger::Get ()->Write (FromDHCPClient, LogWarning,
					"Network configuration incomplete (%x %x %x %x)",
					m_nRxYIAddr, m_nRxSubnetMask, m_nRxRouter, m_nRxDNSServer);

		return FALSE;
	}

	// the router and us must be on the same subnet
	if ((m_nRxYIAddr & m_nRxSubnetMask) != (m_nRxRouter & m_nRxSubnetMask))
	{
		CLogger::Get ()->Write (FromDHCPClient, LogWarning,
					"Not on same network with router (%x %x %x)",
					m_nRxYIAddr, m_nRxRouter, m_nRxSubnetMask);

		return FALSE;
	}

	// lease time must be set
	if (m_nRxIPAddressLeaseTime == 0)
	{
		CLogger::Get ()->Write (FromDHCPClient, LogWarning, "Lease time is not set");

		return FALSE;
	}

	// calculate valid times if one is not set
	if (m_nRxRenewalTimeValue == 0)
	{
		if (m_nRxRebindingTimeValue == 0)
		{
			m_nRxRenewalTimeValue   = m_nRxIPAddressLeaseTime / 2;
			m_nRxRebindingTimeValue = m_nRxIPAddressLeaseTime - m_nRxIPAddressLeaseTime / 8; // 0.875
		}
		else
		{
			m_nRxRenewalTimeValue =   m_nRxRebindingTimeValue
						- (m_nRxIPAddressLeaseTime - m_nRxRebindingTimeValue);
		}
	}
	else
	{
		if (m_nRxRebindingTimeValue == 0)
		{
			m_nRxRebindingTimeValue =   m_nRxRenewalTimeValue
						  + (m_nRxIPAddressLeaseTime - m_nRxRenewalTimeValue) / 2;
		}
	}

	// ensure that RenewalTime <= RebindingTime <= LeaseTime
	if (!(   m_nRxRenewalTimeValue   <= m_nRxRebindingTimeValue
	      && m_nRxRebindingTimeValue <= m_nRxIPAddressLeaseTime))
	{
		CLogger::Get ()->Write (FromDHCPClient, LogWarning, "Time value invalid (%u %u %u)", 
					m_nRxRenewalTimeValue, m_nRxRebindingTimeValue, m_nRxIPAddressLeaseTime);

		return FALSE;
	}

	return TRUE;
}

u32 CDHCPClient::GetXID (void) const
{
	assert (m_pNetSubSystem != 0);
	const u8 *pMACAddress =  m_pNetSubSystem->GetNetDeviceLayer ()->GetMACAddress ()->Get ();
	assert (pMACAddress != 0);

	return GetUnaligned (pMACAddress+2) + CTimer::Get ()->GetClockTicks ();
}

u32 CDHCPClient::GetUnaligned (const void *pPointer)
{
	const u8 *pByte = (const u8 *) pPointer;
	assert (pByte != 0);

	u32 nResult  =       pByte[0];
	nResult     |= (u32) pByte[1] << 8;
	nResult     |= (u32) pByte[2] << 16;
	nResult     |= (u32) pByte[3] << 24;

	return nResult;
}

void CDHCPClient::SetUnaligned (void *pPointer, u32 nValue)
{
	u8 *pByte = (u8 *) pPointer;
	assert (pByte != 0);

	pByte[0] =  nValue        & 0xFF;
	pByte[1] = (nValue >> 8)  & 0xFF;
	pByte[2] = (nValue >> 16) & 0xFF;
	pByte[3] = (nValue >> 24) & 0xFF;
}
