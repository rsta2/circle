//
// udpconnection.cpp
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

CUDPConnection::CUDPConnection (CNetConfig	*pNetConfig,
				CNetworkLayer	*pNetworkLayer,
				CIPAddress	&rForeignIP,
				u16		 nForeignPort,
				u16		 nOwnPort)
:	CNetConnection (pNetConfig, pNetworkLayer, rForeignIP, nForeignPort, nOwnPort, IPPROTO_UDP),
	m_bOpen (TRUE)
{
}

#if 0

CUDPConnection::CUDPConnection (CNetConfig	*pNetConfig,
				CNetworkLayer	*pNetworkLayer,
				u16		 nOwnPort)
:	CNetConnection (pNetConfig, pNetworkLayer, nOwnPort, IPPROTO_UDP),
	m_bOpen (TRUE)
{
}

#endif

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

	assert (pBuffer != 0);
	return m_RxQueue.Dequeue (pBuffer);
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
	if (   nProtocol   != IPPROTO_UDP
	    || m_ForeignIP != rSenderIP)
	{
		return 0;
	}
	
	if (nLength <= sizeof (TUDPHeader))
	{
		return 0;
	}
	TUDPHeader *pHeader = (TUDPHeader *) pPacket;

	if (   m_nOwnPort     != le2be16 (pHeader->nDestPort)
	    || m_nForeignPort != le2be16 (pHeader->nSourcePort))
	{
		return 0;
	}

	if (nLength < le2be16 (pHeader->nLength))
	{
		return -1;
	}
	
	if (   pHeader->nChecksum != UDP_CHECKSUM_NONE
	    && m_Checksum.Calculate (pPacket, nLength) != CHECKSUM_OK)
	{
		return -1;
	}

	nLength -= sizeof (TUDPHeader);
	assert (nLength > 0);

	m_RxQueue.Enqueue ((u8 *) pPacket + sizeof (TUDPHeader), nLength);

	return 1;
}
