//
// udpconnection.cpp
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
#include <circle/net/udpconnection.h>
#include <circle/net/error.h>
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
				const CIPAddress &rForeignIP,
				u16		 nForeignPort,
				u16		 nOwnPort)
:	CNetConnection (pNetConfig, pNetworkLayer, rForeignIP, nForeignPort, nOwnPort, IPPROTO_UDP),
	m_bOpen (TRUE),
	m_bActiveOpen (TRUE),
	m_nReceiveTimeout (0),
	m_bBroadcastsAllowed (FALSE),
	m_pHostGroup (0),
	m_nErrno (0)
{
}

CUDPConnection::CUDPConnection (CNetConfig	*pNetConfig,
				CNetworkLayer	*pNetworkLayer,
				u16		 nOwnPort)
:	CNetConnection (pNetConfig, pNetworkLayer, nOwnPort, IPPROTO_UDP),
	m_bOpen (TRUE),
	m_bActiveOpen (FALSE),
	m_nReceiveTimeout (0),
	m_bBroadcastsAllowed (FALSE),
	m_pHostGroup (0),
	m_nErrno (0)
{
}

CUDPConnection::~CUDPConnection (void)
{
	assert (!m_bOpen);
	assert (!m_pHostGroup);
}

int CUDPConnection::Connect (void)
{
	assert (m_bOpen);

	return 0;
}

int CUDPConnection::Accept (CIPAddress *pForeignIP, u16 *pForeignPort)
{
	return -NET_ERROR_OPERATION_NOT_SUPPORTED;
}

int CUDPConnection::Close (void)
{
	if (!m_bOpen)
	{
		return -NET_ERROR_NOT_CONNECTED;
	}

	if (m_pHostGroup != 0)
	{
		assert (m_pNetworkLayer != 0);
		m_pNetworkLayer->LeaveHostGroup (*m_pHostGroup);

		delete m_pHostGroup;
		m_pHostGroup = 0;
	}

	m_bOpen = FALSE;

	return 0;
}

int CUDPConnection::Send (CNetBuffer *pNetBuffer, int nFlags)
{
	if (m_nErrno < 0)
	{
		int nErrno = m_nErrno;
		m_nErrno = 0;

		return nErrno;
	}

	if (!m_bActiveOpen)
	{
		return -NET_ERROR_OPERATION_NOT_SUPPORTED;
	}

	if (   nFlags != 0
	    && nFlags != MSG_DONTWAIT)
	{
		return -NET_ERROR_INVALID_VALUE;
	}

	assert (pNetBuffer != 0);
	int nLength = pNetBuffer->GetLength ();
	unsigned nPacketLength = sizeof (TUDPHeader) + nLength;		// may wrap
	if (   nPacketLength <= sizeof (TUDPHeader)
	    || nPacketLength > FRAME_BUFFER_SIZE)
	{
		return -NET_ERROR_INVALID_VALUE;
	}

	assert (m_pNetConfig != 0);
	if (   !m_bBroadcastsAllowed
	    && (   m_ForeignIP.IsBroadcast ()
	        || m_ForeignIP == *m_pNetConfig->GetBroadcastAddress ()))
	{
		return -NET_ERROR_PERMISSION_DENIED;
	}

	TUDPHeader *pHeader = (TUDPHeader *) pNetBuffer->AddHeader (sizeof (TUDPHeader));
	assert (pHeader != 0);

	pHeader->nSourcePort = le2be16 (m_nOwnPort);
	pHeader->nDestPort   = le2be16 (m_nForeignPort);
	pHeader->nLength     = le2be16 (nPacketLength);
	pHeader->nChecksum   = 0;
	
	m_Checksum.SetSourceAddress (*m_pNetConfig->GetIPAddress ());
	m_Checksum.SetDestinationAddress (m_ForeignIP);
	pHeader->nChecksum = m_Checksum.Calculate (pHeader, nPacketLength);

	assert (m_pNetworkLayer != 0);
	boolean bOK = m_pNetworkLayer->Send (m_ForeignIP, pNetBuffer, IPPROTO_UDP);
	
	return bOK ? nLength : -NET_ERROR_IO;
}

int CUDPConnection::Receive (CNetBuffer **ppNetBuffer, int nFlags)
{
	assert (ppNetBuffer != 0);
	do
	{
		if (m_nErrno < 0)
		{
			int nErrno = m_nErrno;
			m_nErrno = 0;

			return nErrno;
		}

		*ppNetBuffer = m_RxQueue.Dequeue ();
		if (*ppNetBuffer == 0)
		{
			if (nFlags == MSG_DONTWAIT)
			{
				return 0;
			}

			m_Event.Clear ();

			if (m_nReceiveTimeout == 0)
			{
				m_Event.Wait ();
			}
			else
			{
				if (m_Event.WaitWithTimeout (m_nReceiveTimeout))
				{
					m_nErrno = -1;
				}
			}

			if (m_nErrno < 0)
			{
				int nErrno = m_nErrno;
				m_nErrno = 0;

				return nErrno;
			}
		}
	}
	while (*ppNetBuffer == 0);

	size_t nLength = (*ppNetBuffer)->GetLength ();
	assert (nLength <= FRAME_BUFFER_SIZE);

	return nLength;
}

int CUDPConnection::SendTo (CNetBuffer *pNetBuffer, int nFlags,
			    const CIPAddress &rForeignIP, u16 nForeignPort)
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
		return Send (pNetBuffer, nFlags);
	}

	if (   nFlags != 0
	    && nFlags != MSG_DONTWAIT)
	{
		return -NET_ERROR_INVALID_VALUE;
	}

	assert (pNetBuffer != 0);
	int nLength = pNetBuffer->GetLength ();
	unsigned nPacketLength = sizeof (TUDPHeader) + nLength;		// may wrap
	if (   nPacketLength <= sizeof (TUDPHeader)
	    || nPacketLength > FRAME_BUFFER_SIZE)
	{
		return -NET_ERROR_INVALID_VALUE;
	}

	assert (m_pNetConfig != 0);
	if (   !m_bBroadcastsAllowed
	    && (   rForeignIP.IsBroadcast ()
	        || rForeignIP == *m_pNetConfig->GetBroadcastAddress ()))
	{
		return -NET_ERROR_PERMISSION_DENIED;
	}

	TUDPHeader *pHeader = (TUDPHeader *) pNetBuffer->AddHeader (sizeof (TUDPHeader));
	assert (pHeader != 0);

	pHeader->nSourcePort = le2be16 (m_nOwnPort);
	pHeader->nDestPort   = le2be16 (nForeignPort);
	pHeader->nLength     = le2be16 (nPacketLength);
	pHeader->nChecksum   = 0;
	
	m_Checksum.SetSourceAddress (*m_pNetConfig->GetIPAddress ());
	m_Checksum.SetDestinationAddress (rForeignIP);
	pHeader->nChecksum = m_Checksum.Calculate (pHeader, nPacketLength);

	assert (m_pNetworkLayer != 0);
	boolean bOK = m_pNetworkLayer->Send (rForeignIP, pNetBuffer, IPPROTO_UDP);
	
	return bOK ? nLength : -NET_ERROR_IO;
}

int CUDPConnection::ReceiveFrom (CNetBuffer **ppNetBuffer, int nFlags,
				 CIPAddress *pForeignIP, u16 *pForeignPort)
{
	assert (ppNetBuffer != 0);
	do
	{
		if (m_nErrno < 0)
		{
			int nErrno = m_nErrno;
			m_nErrno = 0;

			return nErrno;
		}

		*ppNetBuffer = m_RxQueue.Dequeue ();
		if (*ppNetBuffer == 0)
		{
			if (nFlags == MSG_DONTWAIT)
			{
				return 0;
			}

			m_Event.Clear ();

			if (m_nReceiveTimeout == 0)
			{
				m_Event.Wait ();
			}
			else
			{
				if (m_Event.WaitWithTimeout (m_nReceiveTimeout))
				{
					m_nErrno = -1;
				}
			}

			if (m_nErrno < 0)
			{
				int nErrno = m_nErrno;
				m_nErrno = 0;

				return nErrno;
			}
		}
	}
	while (*ppNetBuffer == 0);

	size_t nLength = (*ppNetBuffer)->GetLength ();
	assert (nLength <= FRAME_BUFFER_SIZE);

	TUDPPrivateData *pData = (TUDPPrivateData *) (*ppNetBuffer)->GetPrivateData ();
	assert (pData != 0);
	if (   pForeignIP != 0
	    && pForeignPort != 0)
	{
		pForeignIP->Set (pData->SourceAddress);
		*pForeignPort = pData->nSourcePort;
	}

	return nLength;
}

int CUDPConnection::SetOptionReceiveTimeout (unsigned nMicroSeconds)
{
	m_nReceiveTimeout = nMicroSeconds;

	return 0;
}

int CUDPConnection::SetOptionSendTimeout (unsigned nMicroSeconds)
{
	return 0;
}

int CUDPConnection::SetOptionBroadcast (boolean bAllowed)
{
	m_bBroadcastsAllowed = bAllowed;

	return 0;
}

int CUDPConnection::SetOptionAddMembership (const CIPAddress &rGroupAddress)
{
	if (m_pHostGroup != 0)
	{
		return -NET_ERROR_IS_CONNECTED;
	}

	if (!rGroupAddress.IsMulticast ())
	{
		return -NET_ERROR_INVALID_VALUE;
	}

	assert (m_pNetworkLayer != 0);
	if (!m_pNetworkLayer->JoinHostGroup (rGroupAddress))
	{
		return -NET_ERROR_IO;
	}

	m_pHostGroup = new CIPAddress (rGroupAddress);
	assert (m_pHostGroup != 0);

	return 0;
}

int CUDPConnection::SetOptionDropMembership (const CIPAddress &rGroupAddress)
{
	if (m_pHostGroup == 0)
	{
		return -NET_ERROR_NOT_CONNECTED;
	}

	if (*m_pHostGroup != rGroupAddress)
	{
		return -NET_ERROR_INVALID_VALUE;
	}

	delete m_pHostGroup;
	m_pHostGroup = 0;

	assert (m_pNetworkLayer != 0);
#ifndef NDEBUG
	boolean bOK =
#endif
		m_pNetworkLayer->LeaveHostGroup (rGroupAddress);
	assert (bOK);

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

int CUDPConnection::PacketReceived (CNetBuffer *pPacket,
				    CIPAddress &rSenderIP, CIPAddress &rReceiverIP, int nProtocol)
{
	if (nProtocol != IPPROTO_UDP)
	{
		return 0;
	}

	assert (pPacket != 0);
	size_t nLength = pPacket->GetLength ();
	if (nLength <= sizeof (TUDPHeader))
	{
		delete pPacket;

		return -1;
	}
	TUDPHeader *pHeader = (TUDPHeader *) pPacket->GetPtr ();
	assert (pHeader != 0);

	if (m_nOwnPort != be2le16 (pHeader->nDestPort))
	{
		return 0;
	}

	if (rSenderIP.IsMulticast ())
	{
		delete pPacket;

		return -1;
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
		    && !m_ForeignIP.IsMulticast ()
		    && !m_ForeignIP.IsBroadcast ()
		    && m_ForeignIP != *m_pNetConfig->GetBroadcastAddress ())
		{
			return 0;
		}
	}

	if (nLength < be2le16 (pHeader->nLength))
	{
		delete pPacket;

		return -1;
	}
	
	if (pHeader->nChecksum != UDP_CHECKSUM_NONE)
	{
		m_Checksum.SetSourceAddress (rSenderIP);
		m_Checksum.SetDestinationAddress (rReceiverIP);

		if (m_Checksum.Calculate (pHeader, nLength) != CHECKSUM_OK)
		{
			delete pPacket;

			return -1;
		}
	}

	if (   !m_bBroadcastsAllowed
	    && (   rReceiverIP.IsBroadcast ()
	        || rReceiverIP == *m_pNetConfig->GetBroadcastAddress ()))
	{
		delete pPacket;

		return 1;
	}

	if (   rReceiverIP.IsMulticast ()
	    && (   m_pHostGroup == 0
	        || *m_pHostGroup != rReceiverIP))
	{
		return 0;
	}

	pPacket->RemoveHeader (sizeof (TUDPHeader));

	TUDPPrivateData Data;
	rSenderIP.CopyTo (Data.SourceAddress);
	Data.nSourcePort = nSourcePort;
	pPacket->SetPrivateData (&Data, sizeof Data);

	m_RxQueue.Enqueue (pPacket);

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

	m_nErrno =   Type == ICMPNotificationDestUnreach
		   ? -NET_ERROR_DESTINATION_UNREACHABLE
		   : -NET_ERROR_PROTOCOL_ERROR;

	m_Event.Set ();

	return 1;
}

CNetConnection::TStatus CUDPConnection::GetStatus (void) const
{
	TStatus Status = {FALSE, FALSE, FALSE, FALSE};

	if (!m_bOpen)
	{
		return Status;
	}

	Status.bConnected = TRUE;

	if (   m_nErrno < 0
	    || !m_RxQueue.IsEmpty ())
	{
		Status.bRxReady = TRUE;
	}

	Status.bTxReady = TRUE;

	return Status;
}
