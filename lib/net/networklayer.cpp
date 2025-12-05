//
// networklayer.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2025  R. Stange <rsta2@gmx.net>
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
#include <circle/net/networklayer.h>
#include <circle/net/checksumcalculator.h>
#include <circle/net/in.h>
#include <circle/util.h>
#include <assert.h>

CNetworkLayer::CNetworkLayer (CNetConfig *pNetConfig, CLinkLayer *pLinkLayer)
:	m_pNetConfig (pNetConfig),
	m_pLinkLayer (pLinkLayer),
	m_pICMPHandler (0),
	m_pIGMPHandler (0),
	m_pICMPRxQueue2 (0)
{
	assert (m_pNetConfig != 0);
	assert (m_pLinkLayer != 0);
}

CNetworkLayer::~CNetworkLayer (void)
{
	delete m_pICMPRxQueue2;
	m_pICMPRxQueue2 = 0;

	delete m_pIGMPHandler;
	m_pIGMPHandler = 0;

	delete m_pICMPHandler;
	m_pICMPHandler = 0;

	m_pLinkLayer = 0;
	m_pNetConfig = 0;
}

boolean CNetworkLayer::Initialize (void)
{
	assert (m_pICMPHandler == 0);
	m_pICMPHandler = new CICMPHandler (m_pNetConfig, this, &m_ICMPRxQueue, &m_ICMPNotificationQueue);
	assert (m_pICMPHandler != 0);

	assert (m_pIGMPHandler == 0);
	m_pIGMPHandler = new CIGMPHandler (m_pNetConfig, this, m_pLinkLayer, &m_IGMPRxQueue);
	assert (m_pIGMPHandler != 0);

	return TRUE;
}

void CNetworkLayer::Process (void)
{
	assert (m_pNetConfig != 0);
	const CIPAddress *pOwnIPAddress = m_pNetConfig->GetIPAddress ();
	assert (pOwnIPAddress != 0);

	CNetBuffer *pNetBuffer;
	assert (m_pLinkLayer != 0);
	while ((pNetBuffer = m_pLinkLayer->Receive ()) != 0)
	{
		size_t nResultLength = pNetBuffer->GetLength ();
		if (nResultLength <= sizeof (TIPHeader))
		{
			delete pNetBuffer;

			continue;
		}
		TIPHeader *pHeader = (TIPHeader *) pNetBuffer->GetPtr ();

		unsigned nHeaderLength = pHeader->nVersionIHL & 0xF;
		if (   nHeaderLength < IP_HEADER_LENGTH_DWORD_MIN
		    || nHeaderLength > IP_HEADER_LENGTH_DWORD_MAX)
		{
			delete pNetBuffer;

			continue;
		}
		nHeaderLength *= 4;
		if (nResultLength <= nHeaderLength)
		{
			delete pNetBuffer;

			continue;
		}

		if (   CChecksumCalculator::SimpleCalculate (pHeader, nHeaderLength) != CHECKSUM_OK
		    || (pHeader->nVersionIHL >> 4) != IP_VERSION)
		{
			delete pNetBuffer;

			continue;
		}

		CIPAddress IPAddressDestination (pHeader->DestinationAddress);
		if (!pOwnIPAddress->IsNull ())
		{
			if (   *pOwnIPAddress != IPAddressDestination
			    && !IPAddressDestination.IsBroadcast ()
			    && *m_pNetConfig->GetBroadcastAddress () != IPAddressDestination
			    && !IPAddressDestination.IsMulticast ())
			{
				delete pNetBuffer;

				continue;
			}
		}
		else
		{
			if (!IPAddressDestination.IsBroadcast ())
			{
				delete pNetBuffer;

				continue;
			}
		}

		if (   (pHeader->nFlagsFragmentOffset & IP_FLAGS_MF)
		    ||    IP_FRAGMENT_OFFSET (le2be16 (pHeader->nFlagsFragmentOffset))
		       != IP_FRAGMENT_OFFSET_FIRST)
		{
			delete pNetBuffer;

			continue;
		}
		
		unsigned nTotalLength = le2be16 (pHeader->nTotalLength);
		if (nResultLength < nTotalLength)
		{
			delete pNetBuffer;

			continue;
		}

		if (nResultLength > nTotalLength)
		{
			// ignore padding
			pNetBuffer->RemoveTrailer (nResultLength - nTotalLength);
		}

		TNetworkPrivateData Param;
		Param.nProtocol = pHeader->nProtocol;
		memcpy (Param.SourceAddress, pHeader->SourceAddress, IP_ADDRESS_SIZE);
		memcpy (Param.DestinationAddress, pHeader->DestinationAddress, IP_ADDRESS_SIZE);

		pNetBuffer->SetPrivateData (&Param, sizeof Param);

		pNetBuffer->RemoveHeader (nHeaderLength);

		if (pHeader->nProtocol == IPPROTO_ICMP)
		{
			if (m_pICMPRxQueue2 != 0)
			{
				m_pICMPRxQueue2->Enqueue (new CNetBuffer (*pNetBuffer));
			}

			m_ICMPRxQueue.Enqueue (pNetBuffer);
		}
		else if (pHeader->nProtocol == IPPROTO_IGMP)
		{
			m_IGMPRxQueue.Enqueue (pNetBuffer);
		}
		else
		{
			m_RxQueue.Enqueue (pNetBuffer);
		}
	}

	assert (m_pICMPHandler != 0);
	m_pICMPHandler->Process ();

	assert (m_pIGMPHandler != 0);
	m_pIGMPHandler->Process ();
}

boolean CNetworkLayer::Send (const CIPAddress &rReceiver, CNetBuffer *pNetBuffer,
			     int nProtocol, boolean bRouterAlert)
{
	static const u8 RouterAlertOption[] =
	{
		0b1'00'10100,	// Copied, Control, Router Alert
		0x04,		// Length
		0x00, 0x00	// Multicast Listener Discovery
	};

	assert (pNetBuffer != 0);
	unsigned nHeaderLength = sizeof (TIPHeader) + (bRouterAlert ? sizeof RouterAlertOption : 0);
	unsigned nPacketLength = nHeaderLength + pNetBuffer->GetLength ();	// may wrap
	if (   nPacketLength <= nHeaderLength
	    || nPacketLength > FRAME_BUFFER_SIZE)
	{
		delete pNetBuffer;

		return FALSE;
	}

	TIPHeader *pHeader = (TIPHeader *) pNetBuffer->AddHeader (nHeaderLength);

	pHeader->nVersionIHL          = IP_VERSION << 4 | nHeaderLength / 4;
	pHeader->nTypeOfService       = IP_TOS_ROUTINE;
	pHeader->nTotalLength         = le2be16 ((u16) nPacketLength);
	pHeader->nIdentification      = BE (IP_IDENTIFICATION_DEFAULT);
	pHeader->nFlagsFragmentOffset = IP_FLAGS_DF | BE (IP_FRAGMENT_OFFSET_FIRST);
	pHeader->nTTL                 = rReceiver.IsMulticast () ? IP_TTL_MULTICAST : IP_TTL_DEFAULT;
	pHeader->nProtocol            = (u8) nProtocol;

	if (bRouterAlert)
	{
		memcpy (pHeader+1, RouterAlertOption, sizeof RouterAlertOption);
	}

	assert (m_pNetConfig != 0);
	const CIPAddress *pOwnIPAddress = m_pNetConfig->GetIPAddress ();
	assert (pOwnIPAddress != 0);

	pOwnIPAddress->CopyTo (pHeader->SourceAddress);

	rReceiver.CopyTo (pHeader->DestinationAddress);

	pHeader->nHeaderChecksum = 0;
	pHeader->nHeaderChecksum = CChecksumCalculator::SimpleCalculate (pHeader, nHeaderLength);

	if (   pOwnIPAddress->IsNull ()
	    && !rReceiver.IsBroadcast ())
	{
		SendFailed (ICMP_CODE_DEST_NET_UNREACH, pNetBuffer);

		return FALSE;
	}

	CIPAddress GatewayIP;
	const CIPAddress *pNextHop = &rReceiver;
	if (   !rReceiver.IsMulticast ()
	    && !pOwnIPAddress->OnSameNetwork (rReceiver, m_pNetConfig->GetNetMask ()))
	{
		const u8 *pGateway = m_RouteCache.GetRoute (rReceiver.Get ());
		if (pGateway != 0)
		{
			GatewayIP.Set (pGateway);

			pNextHop = &GatewayIP;
		}
		else
		{
			pNextHop = m_pNetConfig->GetDefaultGateway ();
			if (pNextHop->IsNull ())
			{
				SendFailed (ICMP_CODE_DEST_NET_UNREACH, pNetBuffer);

				return FALSE;
			}
		}
	}
	
	assert (m_pLinkLayer != 0);
	assert (pNextHop != 0);
	return m_pLinkLayer->Send (*pNextHop, pNetBuffer);
}

boolean CNetworkLayer::Send (const CIPAddress &rReceiver, const void *pPacket, unsigned nLength,
			     int nProtocol)
{
	assert (pPacket != 0);
	assert (nLength > 0);
	CNetBuffer *pNetBuffer = new CNetBuffer (CNetBuffer::ICMPSend, nLength, pPacket);
	assert (pNetBuffer != 0);

	return Send (rReceiver, pNetBuffer, nProtocol);
}

CNetBuffer *CNetworkLayer::Receive (CIPAddress *pSender, CIPAddress *pReceiver, int *pProtocol)
{
	CNetBuffer *pNetBuffer = m_RxQueue.Dequeue ();
	if (pNetBuffer == 0)
	{
		return 0;
	}

	TNetworkPrivateData *pData = (TNetworkPrivateData *) pNetBuffer->GetPrivateData ();
	assert (pData != 0);

	assert (pProtocol != 0);
	*pProtocol = pData->nProtocol;

	assert (pSender != 0);
	pSender->Set (pData->SourceAddress);

	assert (pReceiver != 0);
	pReceiver->Set (pData->DestinationAddress);

	return pNetBuffer;
}

boolean CNetworkLayer::ReceiveNotification (TICMPNotificationType *pType,
					    CIPAddress *pSender, CIPAddress *pReceiver,
					    u16 *pSendPort, u16 *pReceivePort,
					    int *pProtocol)
{
	TICMPNotification Notification;
	unsigned nLength = m_ICMPNotificationQueue.Dequeue (&Notification);
	if (nLength == 0)
	{
		return FALSE;
	}
	assert (nLength == sizeof Notification);

	assert (pType != 0);
	*pType = Notification.Type;

	assert (pProtocol != 0);
	*pProtocol = Notification.nProtocol;

	assert (pSender != 0);
	pSender->Set (Notification.SourceAddress);

	assert (pReceiver != 0);
	pReceiver->Set (Notification.DestinationAddress);

	assert (pSendPort != 0);
	*pSendPort = Notification.nSourcePort;

	assert (pReceivePort != 0);
	*pReceivePort = Notification.nDestinationPort;

	return TRUE;
}

void CNetworkLayer::EnableReceiveICMP (boolean bEnable)
{
	if (bEnable)
	{
		if (m_pICMPRxQueue2 == 0)
		{
			m_pICMPRxQueue2 = new CNetBufferQueue;
			assert (m_pICMPRxQueue2 != 0);
		}
	}
	else
	{
		if (m_pICMPRxQueue2 != 0)
		{
			CNetBuffer *pNetBuffer;
			while ((pNetBuffer = m_pICMPRxQueue2->Dequeue ()) != 0)
			{
				delete pNetBuffer;
			}

			delete m_pICMPRxQueue2;
			m_pICMPRxQueue2 = 0;
		}
	}
}

boolean CNetworkLayer::ReceiveICMP (void *pBuffer, unsigned *pResultLength,
				    CIPAddress *pSender, CIPAddress *pReceiver)
{
	if (m_pICMPRxQueue2 == 0)
	{
		return FALSE;
	}

	CNetBuffer *pNetBuffer = m_pICMPRxQueue2->Dequeue ();
	if (pNetBuffer == 0)
	{
		return FALSE;
	}

	assert (pBuffer != 0);
	memcpy (pBuffer, pNetBuffer->GetPtr (), pNetBuffer->GetLength ());

	assert (pResultLength != 0);
	*pResultLength = pNetBuffer->GetLength ();

	TNetworkPrivateData *pData = (TNetworkPrivateData *) pNetBuffer->GetPrivateData ();
	assert (pData != 0);

	assert (pData->nProtocol == IPPROTO_ICMP);

	assert (pSender != 0);
	pSender->Set (pData->SourceAddress);

	assert (pReceiver != 0);
	pReceiver->Set (pData->DestinationAddress);

	delete pNetBuffer;

	return TRUE;
}

boolean CNetworkLayer::JoinHostGroup (const CIPAddress &rGroupAddress)
{
	assert (m_pIGMPHandler != 0);
	return m_pIGMPHandler->JoinHostGroup (rGroupAddress);
}

boolean CNetworkLayer::LeaveHostGroup (const CIPAddress &rGroupAddress)
{
	assert (m_pIGMPHandler != 0);
	return m_pIGMPHandler->LeaveHostGroup (rGroupAddress);
}

void CNetworkLayer::AddRoute (const u8 *pDestIP, const u8 *pGatewayIP)
{
	m_RouteCache.AddRoute (pDestIP, pGatewayIP);
}

const u8 *CNetworkLayer::GetGateway (const u8 *pDestIP) const
{
	const u8 *pGateway = m_RouteCache.GetRoute (pDestIP);
	if (pGateway != 0)
	{
		return pGateway;
	}

	assert (m_pNetConfig != 0);
	const CIPAddress *pDefaultGateway = m_pNetConfig->GetDefaultGateway ();
	assert (pDefaultGateway != 0);

	return pDefaultGateway->Get ();
}

void CNetworkLayer::SendFailed (unsigned nICMPCode, CNetBuffer *pReturnedIPPacket)
{
	assert (m_pICMPHandler != 0);
	assert (pReturnedIPPacket != 0);
	m_pICMPHandler->DestinationUnreachable (nICMPCode, pReturnedIPPacket);
}
