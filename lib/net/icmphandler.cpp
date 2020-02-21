//
// icmphandler.cpp
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
#include <circle/net/icmphandler.h>
#include <circle/net/networklayer.h>
#include <circle/net/checksumcalculator.h>
#include <circle/net/in.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/util.h>
#include <circle/macros.h>
#include <assert.h>

struct TICMPHeader
{
	u8	nType;
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
	m_pNotificationQueue (pNotificationQueue)
{
	assert (m_pNetConfig != 0);
	assert (m_pNetworkLayer != 0);
	assert (m_pRxQueue != 0);
	assert (m_pNotificationQueue != 0);
}

CICMPHandler::~CICMPHandler (void)
{
	m_pNotificationQueue = 0;
	m_pRxQueue = 0;
	m_pNetworkLayer = 0;
	m_pNetConfig = 0;
}

void CICMPHandler::Process (void)
{
	u8 Buffer[FRAME_BUFFER_SIZE];
	unsigned nLength;
	void *pParam;
	assert (m_pRxQueue != 0);
	while ((nLength = m_pRxQueue->Dequeue (Buffer, &pParam)) != 0)
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
		TICMPHeader *pICMPHeader = (TICMPHeader *) Buffer;

		if (CChecksumCalculator::SimpleCalculate (Buffer, nLength) != CHECKSUM_OK)
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
				pICMPHeader->nChecksum = CChecksumCalculator::SimpleCalculate (Buffer, nLength);

				assert (m_pNetworkLayer != 0);
				m_pNetworkLayer->Send (SourceIP, Buffer, nLength, IPPROTO_ICMP);
			}

			continue;
		}

		// handle ERROR messages next
		if (nLength <= sizeof (TICMPHeader) + sizeof (TIPHeader))
		{
			continue;
		}
		TIPHeader *pIPHeader = (TIPHeader *) (Buffer + sizeof (TICMPHeader));

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

void CICMPHandler::DestinationUnreachable (unsigned nCode, const void *pReturnedIPPacket,
					   unsigned nLength)
{
	assert (pReturnedIPPacket != 0);
	assert (nLength > sizeof (TIPHeader));
	TIPHeader *pIPHeader = (TIPHeader *) pReturnedIPPacket;

	unsigned nIPHeaderLength = pIPHeader->nVersionIHL & 0xF;
	assert (   nIPHeaderLength >= IP_HEADER_LENGTH_DWORD_MIN
	        && nIPHeaderLength <= IP_HEADER_LENGTH_DWORD_MAX);
	nIPHeaderLength *= 4;

	assert ((pIPHeader->nVersionIHL >> 4) == IP_VERSION);
	assert (*m_pNetConfig->GetIPAddress () == pIPHeader->SourceAddress);
	assert (nLength >= nIPHeaderLength + sizeof (TICMPDataDatagramHeader));
	TICMPDataDatagramHeader *pDatagramHeader =
		(TICMPDataDatagramHeader *) ((u8 *) pIPHeader + nIPHeaderLength);

	const char *pDest;
	switch (nCode)
	{
	case ICMP_CODE_DEST_NET_UNREACH:	pDest = "network ";	break;
	case ICMP_CODE_DEST_HOST_UNREACH:	pDest = "host ";	break;
	default:				pDest = "";		break;
	}

	CString IPString;
	CIPAddress DestinationIP (pIPHeader->DestinationAddress);
	DestinationIP.Format (&IPString);
	CLogger::Get ()->Write (FromICMP, LogDebug, "Destination %sunreachable: %s",
				pDest, (const char *) IPString);

	EnqueueNotification (ICMPNotificationDestUnreach, pIPHeader, pDatagramHeader);
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
