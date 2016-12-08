//
// icmphandler.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>
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
#include <circle/net/icmphandler.h>
#include <circle/net/networklayer.h>
#include <circle/net/checksumcalculator.h>
#include <circle/net/in.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <circle/macros.h>
#include <assert.h>

struct TICMPHeader
{
	u8	nType;
#define ICMP_TYPE_ECHO_REPLY	0
#define ICMP_TYPE_ECHO		8
	#define ICMP_CODE_ECHO			0
#define ICMP_TYPE_DEST_UNREACH	3
	#define ICMP_CODE_DEST_NET_UNREACH	0
	#define ICMP_CODE_DEST_HOST_UNREACH	1
	#define ICMP_CODE_DEST_PROTO_UNREACH	2
	#define ICMP_CODE_DEST_PORT_UNREACH	3
	#define ICMP_CODE_FRAG_REQUIRED		4
	#define ICMP_CODE_SRC_ROUTE_FAIL	5
	#define ICMP_CODE_DEST_NET_UNKNOWN	6
	#define ICMP_CODE_DEST_HOST_UNKNOWN	7
	#define ICMP_CODE_SRC_HOST_ISOLATED	8
	#define ICMP_CODE_NET_ADMIN_PROHIB	9
	#define ICMP_CODE_HOST_ADMIN_PROHIB	0
	#define ICMP_CODE_NET_UNREACH_TOS	11
	#define ICMP_CODE_HOST_UNREACH_TOS	12
	#define ICMP_CODE_COMM_ADMIN_PROHIB	13
	#define ICMP_CODE_HOST_PREC_VIOLAT	14
	#define ICMP_CODE_PREC_CUTOFF_EFFECT	15
#define ICMP_TYPE_REDIRECT	5
	#define ICMP_CODE_REDIRECT_NET		0
	#define ICMP_CODE_REDIRECT_HOST		1
	#define ICMP_CODE_REDIRECT_TOS_NET	2
	#define ICMP_CODE_REDIRECT_TOS_HOST	3
#define ICMP_TYPE_TIME_EXCEED	11
	#define ICMP_CODE_TTL_EXPIRED		0
	#define ICMP_CODE_FRAG_REASS_TIME_EXCEED 1
#define ICMP_TYPE_PARAM_PROBLEM	12
	#define ICMP_CODE_POINTER		0
	#define ICMP_CODE_MISSING_OPTION	1
	#define ICMP_CODE_BAD_LENGTH		2
	u8	nCode;
	u16	nChecksum;
	u8	Parameter[4];		// ICMP_TYPE_REDIRECT: Gateway IP address
					// ICMP_CODE_POINTER: Pointer (in first byte)
					// ICMP_TYPE_ECHO: Identifier and Sequence Number
					// otherwise: unused
}
PACKED;

struct TICMPDataDatagramHeader		// for UDP and TCP
{
	u16	nSourcePort;
	u16	nDestinationPort;
	u32	nUnused;
}
PACKED;

static const char FromICMP[] = "icmp";

CICMPHandler::CICMPHandler (CNetConfig *pNetConfig, CNetworkLayer *pNetworkLayer,
			    CNetQueue *pRxQueue, CNetQueue *pNotificationQueue)
:	m_pNetConfig (pNetConfig),
	m_pNetworkLayer (pNetworkLayer),
	m_pRxQueue (pRxQueue),
	m_pNotificationQueue (pNotificationQueue),
	m_pBuffer (0)
{
	assert (m_pNetConfig != 0);
	assert (m_pNetworkLayer != 0);
	assert (m_pRxQueue != 0);
	assert (m_pNotificationQueue != 0);

	m_pBuffer = new u8[FRAME_BUFFER_SIZE];
	assert (m_pBuffer != 0);
}

CICMPHandler::~CICMPHandler (void)
{
	delete [] m_pBuffer;
	m_pBuffer =  0;

	m_pNotificationQueue = 0;
	m_pRxQueue = 0;
	m_pNetworkLayer = 0;
	m_pNetConfig = 0;
}

void CICMPHandler::Process (void)
{
	unsigned nLength;
	void *pParam;
	assert (m_pRxQueue != 0);
	assert (m_pBuffer != 0);
	while ((nLength = m_pRxQueue->Dequeue (m_pBuffer, &pParam)) != 0)
	{
		TNetworkPrivateData *pData = (TNetworkPrivateData *) pParam;
		assert (pData != 0);
		assert (pData->nProtocol == IPPROTO_ICMP);

		CIPAddress SourceIP (pData->SourceAddress);
		CIPAddress DestIP (pData->DestinationAddress);

		delete pData;
		pData = 0;

		assert (m_pNetConfig != 0);
		if (   DestIP.IsBroadcast ()
		    || DestIP == *m_pNetConfig->GetBroadcastAddress ())
		{
			continue;
		}

		if (nLength < sizeof (TICMPHeader))
		{
			continue;
		}
		TICMPHeader *pICMPHeader = (TICMPHeader *) m_pBuffer;

		if (CChecksumCalculator::SimpleCalculate (m_pBuffer, nLength) != CHECKSUM_OK)
		{
			continue;
		}

		// handle ECHO requests first
		if (pICMPHeader->nType == ICMP_TYPE_ECHO)
		{
			if (pICMPHeader->nCode == ICMP_CODE_ECHO)
			{
				// packet will be used in place to send it back
				pICMPHeader->nType     = ICMP_TYPE_ECHO_REPLY;
				pICMPHeader->nCode     = ICMP_CODE_ECHO;
				pICMPHeader->nChecksum = 0;
				pICMPHeader->nChecksum = CChecksumCalculator::SimpleCalculate (m_pBuffer, nLength);

				assert (m_pNetworkLayer != 0);
				m_pNetworkLayer->Send (SourceIP, m_pBuffer, nLength, IPPROTO_ICMP);
			}

			continue;
		}

		// handle ERROR messages next
		if (nLength <= sizeof (TICMPHeader) + sizeof (TIPHeader))
		{
			continue;
		}
		TIPHeader *pIPHeader = (TIPHeader *) (m_pBuffer + sizeof (TICMPHeader));

		unsigned nIPHeaderLength = pIPHeader->nVersionIHL & 0xF;
		if (   nIPHeaderLength < IP_HEADER_LENGTH_DWORD_MIN
		    || nIPHeaderLength > IP_HEADER_LENGTH_DWORD_MAX)
		{
			continue;
		}
		nIPHeaderLength *= 4;

		if (   (pIPHeader->nVersionIHL >> 4) != IP_VERSION
		    || *m_pNetConfig->GetIPAddress () != pIPHeader->SourceAddress)
		{
			continue;
		}

		if (nLength < sizeof (TICMPHeader) + nIPHeaderLength + sizeof (TICMPDataDatagramHeader))
		{
			continue;
		}
		TICMPDataDatagramHeader *pDatagramHeader =
			(TICMPDataDatagramHeader *) ((u8 *) pIPHeader + nIPHeaderLength);

		switch (pICMPHeader->nType)
		{
		case ICMP_TYPE_DEST_UNREACH:
			CLogger::Get ()->Write (FromICMP, LogDebug, "Destination unreachable (%u)",
						pICMPHeader->nCode);
			EnqueueNotification (ICMPNotificationDestUnreach, pIPHeader, pDatagramHeader);
			break;

		case ICMP_TYPE_REDIRECT: {
			CIPAddress GatewayIP (pICMPHeader->Parameter);

			// See: RFC 1122 3.2.2.2
			assert (m_pNetworkLayer != 0);
			if (   !GatewayIP.OnSameNetwork (*m_pNetConfig->GetIPAddress (),
							 m_pNetConfig->GetNetMask ())
			    || SourceIP != m_pNetworkLayer->GetGateway (pIPHeader->DestinationAddress))
			{
				break;
			}

			CLogger::Get ()->Write (FromICMP, LogDebug, "Redirect (%u)", pICMPHeader->nCode);

			m_pNetworkLayer->AddRoute (pIPHeader->DestinationAddress, GatewayIP.Get ());
			} break;

		case ICMP_TYPE_TIME_EXCEED:
			CLogger::Get ()->Write (FromICMP, LogWarning, "Time exceeded (%u)",
						pICMPHeader->nCode);
			EnqueueNotification (ICMPNotificationTimeExceed, pIPHeader, pDatagramHeader);
			break;

		case ICMP_TYPE_PARAM_PROBLEM:
			CLogger::Get ()->Write (FromICMP, LogWarning, "Parameter problem (%u)",
						pICMPHeader->nCode);
			EnqueueNotification (ICMPNotificationParamProblem, pIPHeader, pDatagramHeader);
			break;

		default:
			break;
		}
	}
}

void CICMPHandler::EnqueueNotification (TICMPNotificationType Type, TIPHeader *pIPHeader,
					TICMPDataDatagramHeader *pDatagramHeader)
{
	TICMPNotification Notification;

	assert (Type < ICMPNotificationUnknown);
	Notification.Type = Type;

	// IP addresses and ports will be swapped here, because they come from the originating packet

	assert (pIPHeader != 0);
	Notification.nProtocol = pIPHeader->nProtocol;
	memcpy (Notification.SourceAddress, pIPHeader->DestinationAddress, IP_ADDRESS_SIZE);
	memcpy (Notification.DestinationAddress, pIPHeader->SourceAddress, IP_ADDRESS_SIZE);

	assert (pDatagramHeader != 0);
	Notification.nSourcePort = be2le16 (pDatagramHeader->nDestinationPort);
	Notification.nDestinationPort = be2le16 (pDatagramHeader->nSourcePort);

	assert (m_pNotificationQueue != 0);
	m_pNotificationQueue->Enqueue (&Notification, sizeof Notification);
}
