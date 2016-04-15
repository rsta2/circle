//
// udpconnection.cpp
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
#include <circle/net/udpconnection.h>
#include <circle/net/in.h>
#include <circle/macros.h>
#include <circle/util.h>
#include <assert.h>

struct TUDPHeader
{
	u16 	nSourcePort;
	u16 	nDestPort;
	u16 	nLength;
	u16 	nChecksum;
#define UDP_CHECKSUM_NONE	0
}
PACKED;

struct TUDPPrivateData
{
	u8	SourceAddress[IP_ADDRESS_SIZE];
	u16	nSourcePort;
};

CUDPConnection::CUDPConnection (CNetConfig	*pNetConfig,
				CNetworkLayer	*pNetworkLayer,
				CIPAddress	&rForeignIP,
				u16		 nForeignPort,
				u16		 nOwnPort)
:	CNetConnection (pNetConfig, pNetworkLayer, rForeignIP, nForeignPort, nOwnPort, IPPROTO_UDP),
	m_bOpen (TRUE),
	m_bActiveOpen (TRUE)
{
}

CUDPConnection::CUDPConnection (CNetConfig	*pNetConfig,
				CNetworkLayer	*pNetworkLayer,
				u16		 nOwnPort)
:	CNetConnection (pNetConfig, pNetworkLayer, nOwnPort, IPPROTO_UDP),
	m_bOpen (TRUE),
	m_bActiveOpen (FALSE)
{
}

CUDPConnection::~CUDPConnection (void)
{
	assert (!m_bOpen);
}

int CUDPConnection::Connect (void)
{
	assert (m_bOpen);

	return 0;
}

int CUDPConnection::Accept (CIPAddress *pForeignIP, u16 *pForeignPort)
{
	return -1;
}

int CUDPConnection::Close (void)
{
	if (!m_bOpen)
	{
		return -1;
	}
	
	m_bOpen = FALSE;

	return 0;
}
	
int CUDPConnection::Send (const void *pData, unsigned nLength, int nFlags)
{
	if (!m_bActiveOpen)
	{
		return -1;
	}

	if (   nFlags != 0
	    && nFlags != MSG_DONTWAIT)
	{
		return -1;
	}

	unsigned nPacketLength = sizeof (TUDPHeader) + nLength;		// may wrap
	if (   nPacketLength <= sizeof (TUDPHeader)
	    || nPacketLength > FRAME_BUFFER_SIZE)
	{
		return -1;
	}

	u8 *pPacketBuffer = new u8[nPacketLength];
	assert (pPacketBuffer != 0);
	TUDPHeader *pHeader = (TUDPHeader *) pPacketBuffer;

	pHeader->nSourcePort = le2be16 (m_nOwnPort);
	pHeader->nDestPort   = le2be16 (m_nForeignPort);
	pHeader->nLength     = le2be16 (nPacketLength);
	pHeader->nChecksum   = 0;
	
	assert (pData != 0);
	assert (nLength > 0);
	memcpy (pPacketBuffer+sizeof (TUDPHeader), pData, nLength);

	assert (m_pNetConfig != 0);
	m_Checksum.SetSourceAddress (*m_pNetConfig->GetIPAddress ());
	m_Checksum.SetDestinationAddress (m_ForeignIP);
	pHeader->nChecksum = m_Checksum.Calculate (pPacketBuffer, nPacketLength);

	assert (m_pNetworkLayer != 0);
	boolean bOK = m_pNetworkLayer->Send (m_ForeignIP, pPacketBuffer, nPacketLength, IPPROTO_UDP);
	
	delete pPacketBuffer;
	pPacketBuffer = 0;

	return bOK ? nLength : -1;
}

int CUDPConnection::Receive (void *pBuffer, int nFlags)
{
	if (nFlags != MSG_DONTWAIT)
	{
		return -1;
	}

	void *pParam;
	assert (pBuffer != 0);
	unsigned nLength = m_RxQueue.Dequeue (pBuffer, &pParam);
	if (nLength == 0)
	{
		return 0;
	}

	TUDPPrivateData *pData = (TUDPPrivateData *) pParam;
	assert (pData != 0);

	delete pData;

	return nLength;
}

int CUDPConnection::SendTo (const void *pData, unsigned nLength, int nFlags,
			    CIPAddress	&rForeignIP, u16 nForeignPort)
{
	if (m_bActiveOpen)
	{
		// ignore rForeignIP and nForeignPort
		return Send (pData, nLength, nFlags);
	}

	if (   nFlags != 0
	    && nFlags != MSG_DONTWAIT)
	{
		return -1;
	}

	unsigned nPacketLength = sizeof (TUDPHeader) + nLength;		// may wrap
	if (   nPacketLength <= sizeof (TUDPHeader)
	    || nPacketLength > FRAME_BUFFER_SIZE)
	{
		return -1;
	}

	u8 *pPacketBuffer = new u8[nPacketLength];
	assert (pPacketBuffer != 0);
	TUDPHeader *pHeader = (TUDPHeader *) pPacketBuffer;

	pHeader->nSourcePort = le2be16 (m_nOwnPort);
	pHeader->nDestPort   = le2be16 (nForeignPort);
	pHeader->nLength     = le2be16 (nPacketLength);
	pHeader->nChecksum   = 0;
	
	assert (pData != 0);
	assert (nLength > 0);
	memcpy (pPacketBuffer+sizeof (TUDPHeader), pData, nLength);

	assert (m_pNetConfig != 0);
	m_Checksum.SetSourceAddress (*m_pNetConfig->GetIPAddress ());
	m_Checksum.SetDestinationAddress (rForeignIP);
	pHeader->nChecksum = m_Checksum.Calculate (pPacketBuffer, nPacketLength);

	assert (m_pNetworkLayer != 0);
	boolean bOK = m_pNetworkLayer->Send (rForeignIP, pPacketBuffer, nPacketLength, IPPROTO_UDP);
	
	delete pPacketBuffer;
	pPacketBuffer = 0;

	return bOK ? nLength : -1;
}

int CUDPConnection::ReceiveFrom (void *pBuffer, int nFlags, CIPAddress *pForeignIP, u16 *pForeignPort)
{
	if (nFlags != MSG_DONTWAIT)
	{
		return -1;
	}

	void *pParam;
	assert (pBuffer != 0);
	unsigned nLength = m_RxQueue.Dequeue (pBuffer, &pParam);
	if (nLength == 0)
	{
		return 0;
	}

	TUDPPrivateData *pData = (TUDPPrivateData *) pParam;
	assert (pData != 0);

	if (   pForeignIP != 0
	    && pForeignPort != 0)
	{
		pForeignIP->Set (pData->SourceAddress);
		*pForeignPort = pData->nSourcePort;
	}

	delete pData;

	return nLength;
}

boolean CUDPConnection::IsTerminated (void) const
{
	return !m_bOpen;
}
	
void CUDPConnection::Process (void)
{
}

int CUDPConnection::PacketReceived (const void *pPacket, unsigned nLength, CIPAddress &rSenderIP, int nProtocol)
{
	if (nProtocol != IPPROTO_UDP)
	{
		return 0;
	}

	m_Checksum.SetSourceAddress (rSenderIP);

	if (   m_bActiveOpen
	    && m_ForeignIP.IsBroadcast ())
	{
		m_Checksum.SetDestinationAddress (m_ForeignIP);
	}
	else
	{
		if (   m_bActiveOpen
		    && m_ForeignIP != rSenderIP)
		{
			return 0;
		}

		assert (m_pNetConfig != 0);
		m_Checksum.SetDestinationAddress (*m_pNetConfig->GetIPAddress ());
	}
	
	if (nLength <= sizeof (TUDPHeader))
	{
		return 0;
	}
	TUDPHeader *pHeader = (TUDPHeader *) pPacket;

	if (m_nOwnPort != be2le16 (pHeader->nDestPort))
	{
		return 0;
	}

	u16 nSourcePort = be2le16 (pHeader->nSourcePort);
	if (   m_bActiveOpen
	    && m_nForeignPort != nSourcePort)
	{
		return 0;
	}

	if (nLength < be2le16 (pHeader->nLength))
	{
		return -1;
	}
	
	if (   pHeader->nChecksum != UDP_CHECKSUM_NONE
	    && m_Checksum.Calculate (pPacket, nLength) != CHECKSUM_OK)
	{
		if (m_bActiveOpen)
		{
			return -1;
		}

		CIPAddress BroadcastIP;
		BroadcastIP.SetBroadcast ();
		m_Checksum.SetDestinationAddress (BroadcastIP);
		if (m_Checksum.Calculate (pPacket, nLength) != CHECKSUM_OK)
		{
			assert (m_pNetConfig != 0);
			m_Checksum.SetDestinationAddress (*m_pNetConfig->GetBroadcastAddress ());
			if (m_Checksum.Calculate (pPacket, nLength) != CHECKSUM_OK)
			{
				return -1;
			}
		}
	}

	nLength -= sizeof (TUDPHeader);
	assert (nLength > 0);

	TUDPPrivateData *pData = new TUDPPrivateData;
	assert (pData != 0);
	rSenderIP.CopyTo (pData->SourceAddress);
	pData->nSourcePort = nSourcePort;

	m_RxQueue.Enqueue ((u8 *) pPacket + sizeof (TUDPHeader), nLength, pData);

	return 1;
}
