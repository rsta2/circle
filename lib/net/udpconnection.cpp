//
// udpconnection.cpp
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
	m_bActiveOpen (TRUE),
	m_bBroadcastsAllowed (FALSE),
	m_nErrno (0)
{
}

CUDPConnection::CUDPConnection (CNetConfig	*pNetConfig,
				CNetworkLayer	*pNetworkLayer,
				u16		 nOwnPort)
:	CNetConnection (pNetConfig, pNetworkLayer, nOwnPort, IPPROTO_UDP),
	m_bOpen (TRUE),
	m_bActiveOpen (FALSE),
	m_bBroadcastsAllowed (FALSE),
	m_nErrno (0)
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
	if (m_nErrno < 0)
	{
		int nErrno = m_nErrno;
		m_nErrno = 0;

		return nErrno;
	}

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

	assert (m_pNetConfig != 0);
	if (   !m_bBroadcastsAllowed
	    && (   m_ForeignIP.IsBroadcast ()
	        || m_ForeignIP == *m_pNetConfig->GetBroadcastAddress ()))
	{
		return -1;
	}

	u8 PacketBuffer[nPacketLength];
	TUDPHeader *pHeader = (TUDPHeader *) PacketBuffer;

	pHeader->nSourcePort = le2be16 (m_nOwnPort);
	pHeader->nDestPort   = le2be16 (m_nForeignPort);
	pHeader->nLength     = le2be16 (nPacketLength);
	pHeader->nChecksum   = 0;
	
	assert (pData != 0);
	assert (nLength > 0);
	memcpy (PacketBuffer+sizeof (TUDPHeader), pData, nLength);

	m_Checksum.SetSourceAddress (*m_pNetConfig->GetIPAddress ());
	m_Checksum.SetDestinationAddress (m_ForeignIP);
	pHeader->nChecksum = m_Checksum.Calculate (PacketBuffer, nPacketLength);

	assert (m_pNetworkLayer != 0);
	boolean bOK = m_pNetworkLayer->Send (m_ForeignIP, PacketBuffer, nPacketLength, IPPROTO_UDP);
	
	return bOK ? nLength : -1;
}

int CUDPConnection::Receive (void *pBuffer, int nFlags)
{
	void *pParam;
	unsigned nLength;
	do
	{
		if (m_nErrno < 0)
		{
			int nErrno = m_nErrno;
			m_nErrno = 0;

			return nErrno;
		}

		assert (pBuffer != 0);
		nLength = m_RxQueue.Dequeue (pBuffer, &pParam);
		if (nLength == 0)
		{
			if (nFlags == MSG_DONTWAIT)
			{
				return 0;
			}

			m_Event.Clear ();
			m_Event.Wait ();

			if (m_nErrno < 0)
			{
				int nErrno = m_nErrno;
				m_nErrno = 0;

				return nErrno;
			}
		}
	}
	while (nLength == 0);

	TUDPPrivateData *pData = (TUDPPrivateData *) pParam;
	assert (pData != 0);

	delete pData;

	return nLength;
}

int CUDPConnection::SendTo (const void *pData, unsigned nLength, int nFlags,
			    CIPAddress	&rForeignIP, u16 nForeignPort)
{
	if (m_nErrno < 0)
	{
		int nErrno = m_nErrno;
		m_nErrno = 0;

		return nErrno;
	}

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

	assert (m_pNetConfig != 0);
	if (   !m_bBroadcastsAllowed
	    && (   rForeignIP.IsBroadcast ()
	        || rForeignIP == *m_pNetConfig->GetBroadcastAddress ()))
	{
		return -1;
	}

	u8 PacketBuffer[nPacketLength];
	TUDPHeader *pHeader = (TUDPHeader *) PacketBuffer;

	pHeader->nSourcePort = le2be16 (m_nOwnPort);
	pHeader->nDestPort   = le2be16 (nForeignPort);
	pHeader->nLength     = le2be16 (nPacketLength);
	pHeader->nChecksum   = 0;
	
	assert (pData != 0);
	assert (nLength > 0);
	memcpy (PacketBuffer+sizeof (TUDPHeader), pData, nLength);

	m_Checksum.SetSourceAddress (*m_pNetConfig->GetIPAddress ());
	m_Checksum.SetDestinationAddress (rForeignIP);
	pHeader->nChecksum = m_Checksum.Calculate (PacketBuffer, nPacketLength);

	assert (m_pNetworkLayer != 0);
	boolean bOK = m_pNetworkLayer->Send (rForeignIP, PacketBuffer, nPacketLength, IPPROTO_UDP);
	
	return bOK ? nLength : -1;
}

int CUDPConnection::ReceiveFrom (void *pBuffer, int nFlags, CIPAddress *pForeignIP, u16 *pForeignPort)
{
	void *pParam;
	unsigned nLength;
	do
	{
		if (m_nErrno < 0)
		{
			int nErrno = m_nErrno;
			m_nErrno = 0;

			return nErrno;
		}

		assert (pBuffer != 0);
		nLength = m_RxQueue.Dequeue (pBuffer, &pParam);
		if (nLength == 0)
		{
			if (nFlags == MSG_DONTWAIT)
			{
				return 0;
			}

			m_Event.Clear ();
			m_Event.Wait ();

			if (m_nErrno < 0)
			{
				int nErrno = m_nErrno;
				m_nErrno = 0;

				return nErrno;
			}
		}
	}
	while (nLength == 0);

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

int CUDPConnection::SetOptionBroadcast (boolean bAllowed)
{
	m_bBroadcastsAllowed = bAllowed;

	return 0;
}

boolean CUDPConnection::IsConnected (void) const
{
	return FALSE;
}

boolean CUDPConnection::IsTerminated (void) const
{
	return !m_bOpen;
}
	
void CUDPConnection::Process (void)
{
}

int CUDPConnection::PacketReceived (const void *pPacket, unsigned nLength,
				    CIPAddress &rSenderIP, CIPAddress &rReceiverIP, int nProtocol)
{
	if (nProtocol != IPPROTO_UDP)
	{
		return 0;
	}

	if (nLength <= sizeof (TUDPHeader))
	{
		return -1;
	}
	TUDPHeader *pHeader = (TUDPHeader *) pPacket;

	if (m_nOwnPort != be2le16 (pHeader->nDestPort))
	{
		return 0;
	}

	assert (m_pNetConfig != 0);

	u16 nSourcePort = be2le16 (pHeader->nSourcePort);
	if (m_bActiveOpen)
	{
		if (m_nForeignPort != nSourcePort)
		{
			return 0;
		}

		if (   m_ForeignIP != rSenderIP
		    && !m_ForeignIP.IsBroadcast ()
		    && m_ForeignIP != *m_pNetConfig->GetBroadcastAddress ())
		{
			return 0;
		}
	}

	if (nLength < be2le16 (pHeader->nLength))
	{
		return -1;
	}
	
	if (pHeader->nChecksum != UDP_CHECKSUM_NONE)
	{
		m_Checksum.SetSourceAddress (rSenderIP);
		m_Checksum.SetDestinationAddress (rReceiverIP);

		if (m_Checksum.Calculate (pPacket, nLength) != CHECKSUM_OK)
		{
			return -1;
		}
	}

	if (   !m_bBroadcastsAllowed
	    && (   rReceiverIP.IsBroadcast ()
	        || rReceiverIP == *m_pNetConfig->GetBroadcastAddress ()))
	{
		return 1;
	}

	nLength -= sizeof (TUDPHeader);
	assert (nLength > 0);

	TUDPPrivateData *pData = new TUDPPrivateData;
	assert (pData != 0);
	rSenderIP.CopyTo (pData->SourceAddress);
	pData->nSourcePort = nSourcePort;

	m_RxQueue.Enqueue ((u8 *) pPacket + sizeof (TUDPHeader), nLength, pData);

	m_Event.Set ();

	return 1;
}

int CUDPConnection::NotificationReceived (TICMPNotificationType  Type,
					  CIPAddress		&rSenderIP,
					  CIPAddress		&rReceiverIP,
					  u16			 nSendPort,
					  u16			 nReceivePort,
					  int			 nProtocol)
{
	if (nProtocol != IPPROTO_UDP)
	{
		return 0;
	}

	if (m_nOwnPort != nReceivePort)
	{
		return 0;
	}

	assert (m_pNetConfig != 0);
	if (rReceiverIP != *m_pNetConfig->GetIPAddress ())
	{
		return 0;
	}

	if (m_bActiveOpen)
	{
		if (m_nForeignPort != nSendPort)
		{
			return 0;
		}

		if (m_ForeignIP != rSenderIP)
		{
			return 0;
		}
	}

	m_nErrno = -1;

	m_Event.Set ();

	return 1;
}
