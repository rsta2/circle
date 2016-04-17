//
// networklayer.cpp
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
#include <circle/net/networklayer.h>
#include <circle/net/checksumcalculator.h>
#include <circle/net/in.h>
#include <circle/util.h>
#include <circle/macros.h>
#include <assert.h>

struct TIPHeader
{
	u8	nVersionIHL;
#define IP_VERSION			4
#define IP_HEADER_LENGTH_DWORD_MIN	5
#define IP_HEADER_LENGTH_DWORD_MAX	6
	u8	nTypeOfService;
#define IP_TOS_ROUTINE			0
	u16	nTotalLength;
	u16	nIdentification;
#define IP_IDENTIFICATION_DEFAULT	0
	u16	nFlagsFragmentOffset;
#define IP_FRAGMENT_OFFSET(field)	((field) & 0x1F00)
	#define IP_FRAGMENT_OFFSET_FIRST	0
#define IP_FLAGS_DF			(1 << 6)	// valid without BE()
#define IP_FLAGS_MF			(1 << 5)
	u8	nTTL;
#define IP_TTL_DEFAULT			64
	u8	nProtocol;				// see: in.h
	u16	nHeaderChecksum;
	u8	SourceAddress[IP_ADDRESS_SIZE];
	u8	DestinationAddress[IP_ADDRESS_SIZE];
	//u32	nOptionsPadding;			// optional
}
PACKED;

CNetworkLayer::CNetworkLayer (CNetConfig *pNetConfig, CLinkLayer *pLinkLayer)
:	m_pNetConfig (pNetConfig),
	m_pLinkLayer (pLinkLayer),
	m_pICMPHandler (0),
	m_pBuffer (0)
{
	assert (m_pNetConfig != 0);
	assert (m_pLinkLayer != 0);
}

CNetworkLayer::~CNetworkLayer (void)
{
	delete m_pBuffer;
	m_pBuffer = 0;

	delete m_pICMPHandler;
	m_pICMPHandler = 0;

	m_pLinkLayer = 0;
	m_pNetConfig = 0;
}

boolean CNetworkLayer::Initialize (void)
{
	assert (m_pICMPHandler == 0);
	m_pICMPHandler = new CICMPHandler (m_pNetConfig, this, &m_ICMPRxQueue);
	assert (m_pICMPHandler != 0);

	assert (m_pBuffer == 0);
	m_pBuffer = new u8[FRAME_BUFFER_SIZE];
	assert (m_pBuffer != 0);

	return TRUE;
}

void CNetworkLayer::Process (void)
{
	assert (m_pNetConfig != 0);
	const CIPAddress *pOwnIPAddress = m_pNetConfig->GetIPAddress ();
	assert (pOwnIPAddress != 0);

	unsigned nResultLength;
	assert (m_pBuffer != 0);
	assert (m_pLinkLayer != 0);
	while (m_pLinkLayer->Receive (m_pBuffer, &nResultLength))
	{
		if (nResultLength <= sizeof (TIPHeader))
		{
			continue;
		}
		TIPHeader *pHeader = (TIPHeader *) m_pBuffer;

		unsigned nHeaderLength = pHeader->nVersionIHL & 0xF;
		if (   nHeaderLength < IP_HEADER_LENGTH_DWORD_MIN
		    || nHeaderLength > IP_HEADER_LENGTH_DWORD_MAX)
		{
			continue;
		}
		nHeaderLength *= 4;
		if (nResultLength <= nHeaderLength)
		{
			continue;
		}

		if (   CChecksumCalculator::SimpleCalculate (pHeader, nHeaderLength) != CHECKSUM_OK
		    || (pHeader->nVersionIHL >> 4) != IP_VERSION)
		{
			continue;
		}

		CIPAddress IPAddressDestination (pHeader->DestinationAddress);
		if (!pOwnIPAddress->IsNull ())
		{
			if (   *pOwnIPAddress != IPAddressDestination
			    && !IPAddressDestination.IsBroadcast ()
			    && *m_pNetConfig->GetBroadcastAddress () != IPAddressDestination)
			{
				continue;
			}
		}
		else
		{
			if (!IPAddressDestination.IsBroadcast ())
			{
				continue;
			}
		}

		if (   (pHeader->nFlagsFragmentOffset & IP_FLAGS_MF)
		    ||    IP_FRAGMENT_OFFSET (le2be16 (pHeader->nFlagsFragmentOffset))
		       != IP_FRAGMENT_OFFSET_FIRST)
		{
			continue;
		}
		
		unsigned nTotalLength = le2be16 (pHeader->nTotalLength);
		if (nResultLength < nTotalLength)
		{
			continue;
		}
		nResultLength = nTotalLength;		// ignore padding

		TNetworkPrivateData *pParam = new TNetworkPrivateData;
		assert (pParam != 0);
		pParam->nProtocol = pHeader->nProtocol;
		memcpy (pParam->SourceAddress, pHeader->SourceAddress, IP_ADDRESS_SIZE);

		nResultLength -= nHeaderLength;

		if (pHeader->nProtocol == IPPROTO_ICMP)
		{
			m_ICMPRxQueue.Enqueue ((u8 *) m_pBuffer + nHeaderLength, nResultLength, pParam);
		}
		else
		{
			m_RxQueue.Enqueue ((u8 *) m_pBuffer + nHeaderLength, nResultLength, pParam);
		}
	}

	assert (m_pICMPHandler != 0);
	m_pICMPHandler->Process ();
}

boolean CNetworkLayer::Send (const CIPAddress &rReceiver, const void *pPacket, unsigned nLength, int nProtocol)
{
	unsigned nPacketLength = sizeof (TIPHeader) + nLength;		// may wrap
	if (   nPacketLength <= sizeof (TIPHeader)
	    || nPacketLength > FRAME_BUFFER_SIZE)
	{
		return FALSE;
	}

	u8 *pPacketBuffer = new u8[nPacketLength];
	assert (pPacketBuffer != 0);
	TIPHeader *pHeader = (TIPHeader *) pPacketBuffer;

	pHeader->nVersionIHL          = IP_VERSION << 4 | IP_HEADER_LENGTH_DWORD_MIN;
	pHeader->nTypeOfService       = IP_TOS_ROUTINE;
	pHeader->nTotalLength         = le2be16 ((u16) nPacketLength);
	pHeader->nIdentification      = BE (IP_IDENTIFICATION_DEFAULT);
	pHeader->nFlagsFragmentOffset = IP_FLAGS_DF | BE (IP_FRAGMENT_OFFSET_FIRST);
	pHeader->nTTL                 = IP_TTL_DEFAULT;
	pHeader->nProtocol            = (u8) nProtocol;

	assert (m_pNetConfig != 0);
	const CIPAddress *pOwnIPAddress = m_pNetConfig->GetIPAddress ();
	assert (pOwnIPAddress != 0);

	if (   pOwnIPAddress->IsNull ()
	    && !rReceiver.IsBroadcast ())
	{
		delete pPacketBuffer;
		pPacketBuffer = 0;

		return FALSE;
	}

	pOwnIPAddress->CopyTo (pHeader->SourceAddress);

	rReceiver.CopyTo (pHeader->DestinationAddress);

	pHeader->nHeaderChecksum = 0;
	pHeader->nHeaderChecksum = CChecksumCalculator::SimpleCalculate (pHeader, sizeof (TIPHeader));

	assert (pPacket != 0);
	assert (nLength > 0);
	memcpy (pPacketBuffer+sizeof (TIPHeader), pPacket, nLength);

	const CIPAddress *pNextHop = &rReceiver;
	if (!pOwnIPAddress->OnSameNetwork (rReceiver, m_pNetConfig->GetNetMask ()))
	{
		pNextHop = m_pNetConfig->GetDefaultGateway ();
		if (pNextHop->IsNull ())
		{
			delete pPacketBuffer;
			pPacketBuffer = 0;

			return FALSE;
		}
	}
	
	assert (m_pLinkLayer != 0);
	assert (pNextHop != 0);
	boolean bOK = m_pLinkLayer->Send (*pNextHop, pPacketBuffer, nPacketLength);
	
	delete pPacketBuffer;
	pPacketBuffer = 0;

	return bOK;
}

boolean CNetworkLayer::Receive (void *pBuffer, unsigned *pResultLength, CIPAddress *pSender, int *pProtocol)
{
	void *pParam;
	assert (pBuffer != 0);
	assert (pResultLength != 0);
	*pResultLength = m_RxQueue.Dequeue (pBuffer, &pParam);
	if (*pResultLength == 0)
	{
		return FALSE;
	}
	
	TNetworkPrivateData *pData = (TNetworkPrivateData *) pParam;
	assert (pData != 0);

	assert (pProtocol != 0);
	*pProtocol = pData->nProtocol;

	assert (pSender != 0);
	pSender->Set (pData->SourceAddress);

	delete pData;
	pData = 0;
	
	return TRUE;
}
