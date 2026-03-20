//
// socket.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2026  R. Stange <rsta2@gmx.net>
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
#include <circle/net/socket.h>
#include <circle/net/netsubsystem.h>
#include <circle/net/netbuffer.h>
#include <circle/net/in.h>
#include <circle/sched/scheduler.h>
#include <circle/util.h>
#include <assert.h>

CSocket::CSocket (CNetSubSystem *pNetSubSystem, int nProtocol)
:	CNetSocket (pNetSubSystem),
	m_pNetConfig (pNetSubSystem->GetConfig ()),
	m_pTransportLayer (pNetSubSystem->GetTransportLayer ()),
	m_nProtocol (nProtocol),
	m_nOwnPort (0),
	m_hConnection (-1),
	m_nBackLog (0),
	m_bEnableBroadcast (FALSE)
{
	assert (m_pNetConfig != 0);
	assert (m_pTransportLayer != 0);
}

CSocket::CSocket (CSocket &rSocket, int hConnection)
:	CNetSocket (rSocket.GetNetSubSystem ()),
	m_pNetConfig (rSocket.m_pNetConfig),
	m_pTransportLayer (rSocket.m_pTransportLayer),
	m_nProtocol (rSocket.m_nProtocol),
	m_nOwnPort (rSocket.m_nOwnPort),
	m_hConnection (hConnection),
	m_nBackLog (0),
	m_bEnableBroadcast (rSocket.m_bEnableBroadcast)
{
	assert (m_pNetConfig != 0);
	assert (m_pTransportLayer != 0);
}

CSocket::~CSocket (void)
{
	assert (m_pTransportLayer != 0);

	if (m_hConnection >= 0)
	{
		assert (m_nBackLog == 0);
		m_pTransportLayer->Disconnect (m_hConnection);
		m_hConnection = -1;
	}
	else
	{
		for (unsigned i = 0; i < m_nBackLog; i++)
		{
			m_pTransportLayer->Disconnect (m_hListenConnection[i]);
		}
	}

	m_pTransportLayer = 0;
	m_pNetConfig = 0;
}

int CSocket::GetProtocol (void) const
{
	return m_nProtocol;
}

int CSocket::Bind (u16 nOwnPort)
{
	if (m_nOwnPort != 0)
	{
		return -NET_ERROR_INVALID_VALUE;
	}

	if (m_hConnection >= 0)
	{
		return -NET_ERROR_INVALID_VALUE;
	}
	
	m_nOwnPort = nOwnPort;				// TODO: suppress double usage of port
	
	if (m_nProtocol == IPPROTO_UDP)
	{
		m_hConnection = m_pTransportLayer->Bind (m_nOwnPort, m_nProtocol);
		if (m_hConnection < 0)
		{
			return m_hConnection;		// return error code
		}

		if (m_bEnableBroadcast)
		{
			m_bEnableBroadcast = FALSE;
			m_pTransportLayer->SetOptionBroadcast (TRUE, m_hConnection);
		}
	}

	return 0;
}

int CSocket::Connect (const CIPAddress &rForeignIP, u16 nForeignPort)
{
	if (nForeignPort == 0)
	{
		return -NET_ERROR_INVALID_VALUE;
	}

	assert (m_pTransportLayer != 0);

	if (m_hConnection >= 0)
	{
		if (m_nProtocol != IPPROTO_UDP)
		{
			return -NET_ERROR_IS_CONNECTED;
		}

		m_pTransportLayer->Disconnect (m_hConnection);
		m_hConnection = -1;
	}

	assert (m_pNetConfig != 0);
	if (   m_pNetConfig->GetIPAddress ()->IsNull ()		// from null source address
	    && !(   m_nProtocol == IPPROTO_UDP			// only UDP broadcasts are allowed
		 && rForeignIP.IsBroadcast ()))
	{
		return -NET_ERROR_PROTOCOL_NOT_SUPPORTED;
	}

	m_hConnection = m_pTransportLayer->Connect (rForeignIP, nForeignPort, m_nOwnPort, m_nProtocol);

	if (   m_hConnection >= 0
	    && m_bEnableBroadcast)
	{
		m_bEnableBroadcast = FALSE;
		m_pTransportLayer->SetOptionBroadcast (TRUE, m_hConnection);
	}

	return m_hConnection >= 0 ? 0 : m_hConnection;
}

int CSocket::Listen (unsigned nBackLog)
{
	if (m_nProtocol != IPPROTO_TCP)
	{
		return -NET_ERROR_OPERATION_NOT_SUPPORTED;
	}

	if (m_nOwnPort == 0)
	{
		return -NET_ERROR_INVALID_VALUE;
	}

	if (m_hConnection >= 0)
	{
		return -NET_ERROR_INVALID_VALUE;
	}

	if (   nBackLog == 0
	    || nBackLog > SOCKET_MAX_LISTEN_BACKLOG)
	{
		return -NET_ERROR_INVALID_VALUE;
	}

	assert (m_nBackLog == 0);
	m_nBackLog = nBackLog;

	assert (m_pTransportLayer != 0);

	for (unsigned i = 0; i < m_nBackLog; i++)
	{
		m_hListenConnection[i] = m_pTransportLayer->Listen (m_nOwnPort, m_nProtocol);
		assert (m_hListenConnection[i] >= 0);
	}

	return 0;
}

CSocket *CSocket::Accept (CIPAddress *pForeignIP, u16 *pForeignPort)
{
	if (   m_nBackLog == 0
	    || m_nOwnPort == 0)
	{
		return 0;
	}

	assert (m_pTransportLayer != 0);
	assert (m_nBackLog <= SOCKET_MAX_LISTEN_BACKLOG);

	// find a connection, which is already active or will be active first (with lowest handle)
#define INT_MAX		0x7FFFFFFF
	int hConnection = INT_MAX;
	unsigned nIndex = SOCKET_MAX_LISTEN_BACKLOG;
	for (unsigned i = 0; i < m_nBackLog; i++)
	{
		if (m_pTransportLayer->IsConnected (m_hListenConnection[i]))
		{
			hConnection = m_hListenConnection[i];
			nIndex = i;

			break;
		}

		if (m_hListenConnection[i] < hConnection)
		{
			hConnection = m_hListenConnection[i];
			nIndex = i;
		}
	}

	assert (0 <= hConnection && hConnection < INT_MAX);
	assert (nIndex < m_nBackLog);

	CSocket *pNewSocket = 0;

	assert (pForeignIP != 0);
	assert (pForeignPort != 0);

	// blocks until connection is active
	if (m_pTransportLayer->Accept (pForeignIP, pForeignPort, hConnection) >= 0)
	{
		pNewSocket = new CSocket (*this, hConnection);
		assert (pNewSocket != 0);
	}

	// replace the returned connection with a new listening one
	m_hListenConnection[nIndex] = m_pTransportLayer->Listen (m_nOwnPort, m_nProtocol);
	assert (m_hListenConnection[nIndex] >= 0);

	return pNewSocket;
}

int CSocket::Send (const void *pBuffer, unsigned nLength, int nFlags)
{
	if (m_hConnection < 0)
	{
		return -NET_ERROR_NOT_CONNECTED;
	}

	if (nLength == 0)
	{
		return -NET_ERROR_INVALID_VALUE;
	}

	assert (m_pTransportLayer != 0);
	u16 nMSS = m_pTransportLayer->GetMSS (m_hConnection);
	if (   nLength > nMSS
	    && m_nProtocol == IPPROTO_UDP)
	{
		return -NET_ERROR_INVALID_VALUE;
	}

	assert (pBuffer != 0);
	const u8 *p = (const u8 *) pBuffer;
	unsigned nRemaining = nLength;
	while (nRemaining > 0)
	{
		unsigned nPayload = nRemaining;
		int nTempFlags = nFlags;
		if (nRemaining > nMSS)
		{
			nTempFlags |= MSG_MORE;
			nPayload = nMSS;
		}

		CNetBuffer *pNetBuffer = new CNetBuffer (  m_nProtocol == IPPROTO_TCP
							 ? CNetBuffer::TCPSend : CNetBuffer::UDPSend,
							 nPayload, p);
		assert (pNetBuffer != 0);

		int nResult = m_pTransportLayer->Send (pNetBuffer, nTempFlags, m_hConnection);
		if (nResult < 0)
		{
			delete pNetBuffer;

			return nResult;
		}

		p += nPayload;
		nRemaining -= nPayload;

		CScheduler::Get ()->Yield ();
	}

	return nLength;
}

int CSocket::Receive (void *pBuffer, unsigned nLength, int nFlags)
{
	if (m_hConnection < 0)
	{
		return -NET_ERROR_NOT_CONNECTED;
	}
	
	if (nLength == 0)
	{
		return -NET_ERROR_INVALID_VALUE;
	}
	
	assert (m_pTransportLayer != 0);
	CNetBuffer *pNetBuffer = 0;
	int nResult = m_pTransportLayer->Receive (&pNetBuffer, nFlags, m_hConnection);
	if (nResult > 0)
	{
		assert (pNetBuffer != 0);
		assert (pNetBuffer->GetLength () == (unsigned) nResult);
		if (nLength < (unsigned) nResult)
		{
			nResult = nLength;
		}

		assert (pBuffer != 0);
		memcpy (pBuffer, pNetBuffer->GetPtr (), nResult);
	}

	delete pNetBuffer;

	return nResult;
}

int CSocket::SendTo (const void *pBuffer, unsigned nLength, int nFlags,
		     const CIPAddress &rForeignIP, u16 nForeignPort)
{
	assert (m_pTransportLayer != 0);

	if (m_hConnection < 0)
	{
		if (m_nProtocol != IPPROTO_UDP)
		{
			return -NET_ERROR_NOT_CONNECTED;
		}

		// assign ephemeral port
		m_hConnection = m_pTransportLayer->Bind (0, m_nProtocol);
		if (m_hConnection < 0)
		{
			return m_hConnection;		// return error code
		}

		if (m_bEnableBroadcast)
		{
			m_bEnableBroadcast = FALSE;
			m_pTransportLayer->SetOptionBroadcast (TRUE, m_hConnection);
		}
	}

	if (nLength == 0)
	{
		return -NET_ERROR_INVALID_VALUE;
	}
	
	u16 nMSS = m_pTransportLayer->GetMSS (m_hConnection);
	if (   nLength > nMSS
	    && m_nProtocol == IPPROTO_UDP)
	{
		return -NET_ERROR_INVALID_VALUE;
	}

	assert (m_pNetConfig != 0);
	if (m_pNetConfig->GetIPAddress ()->IsNull ())		// from null source address
	{
		return -NET_ERROR_OPERATION_NOT_SUPPORTED;
	}

	if (   m_nProtocol == IPPROTO_UDP
	    && nForeignPort == 0)
	{
		return -NET_ERROR_INVALID_VALUE;
	}

	assert (pBuffer != 0);
	const u8 *p = (const u8 *) pBuffer;
	unsigned nRemaining = nLength;
	while (nRemaining > 0)
	{
		unsigned nPayload = nRemaining;
		int nTempFlags = nFlags;
		if (nRemaining > nMSS)
		{
			nTempFlags |= MSG_MORE;
			nPayload = nMSS;
		}

		CNetBuffer *pNetBuffer = new CNetBuffer (  m_nProtocol == IPPROTO_TCP
							 ? CNetBuffer::TCPSend : CNetBuffer::UDPSend,
							 nPayload, p);
		assert (pNetBuffer != 0);

		int nResult = m_pTransportLayer->SendTo (pNetBuffer, nTempFlags,
							 rForeignIP, nForeignPort, m_hConnection);
		if (nResult < 0)
		{
			delete pNetBuffer;

			return nResult;
		}

		p += nPayload;
		nRemaining -= nPayload;

		CScheduler::Get ()->Yield ();
	}

	return nLength;
}

int CSocket::ReceiveFrom (void *pBuffer, unsigned nLength, int nFlags,
			  CIPAddress *pForeignIP, u16 *pForeignPort)
{
	if (m_hConnection < 0)
	{
		return -NET_ERROR_NOT_CONNECTED;
	}
	
	if (nLength == 0)
	{
		return -NET_ERROR_INVALID_VALUE;
	}
	
	assert (m_pTransportLayer != 0);
	CNetBuffer *pNetBuffer = 0;
	int nResult = m_pTransportLayer->ReceiveFrom (&pNetBuffer, nFlags,
						      pForeignIP, pForeignPort, m_hConnection);
	if (nResult > 0)
	{
		assert (pNetBuffer != 0);
		assert (pNetBuffer->GetLength () == (unsigned) nResult);
		if (nLength < (unsigned) nResult)
		{
			nResult = nLength;
		}

		assert (pBuffer != 0);
		memcpy (pBuffer, pNetBuffer->GetPtr (), nResult);
	}

	delete pNetBuffer;

	return nResult;
}

int CSocket::SetOptionReceiveTimeout (unsigned nMicroSeconds)
{
	if (m_hConnection < 0)
	{
		return -NET_ERROR_NOT_CONNECTED;
	}

	assert (m_pTransportLayer != 0);
	return m_pTransportLayer->SetOptionReceiveTimeout (nMicroSeconds, m_hConnection);
}

int CSocket::SetOptionSendTimeout (unsigned nMicroSeconds)
{
	if (m_hConnection < 0)
	{
		return -NET_ERROR_NOT_CONNECTED;
	}

	assert (m_pTransportLayer != 0);
	return m_pTransportLayer->SetOptionSendTimeout (nMicroSeconds, m_hConnection);
}

int CSocket::SetOptionBroadcast (boolean bAllowed)
{
	if (m_nProtocol != IPPROTO_UDP)		// ignore, when called for TCP
	{
		return 0;
	}

	if (m_hConnection < 0)
	{
		// delayed operation after Bind(), Connect() or SendTo()
		m_bEnableBroadcast = bAllowed;

		return 0;
	}

	assert (m_pTransportLayer != 0);
	return m_pTransportLayer->SetOptionBroadcast (bAllowed, m_hConnection);
}

int CSocket::SetOptionAddMembership (const CIPAddress &rGroupAddress)
{
	if (m_hConnection < 0)
	{
		return -NET_ERROR_NOT_CONNECTED;
	}

	if (m_nProtocol != IPPROTO_UDP)
	{
		return -NET_ERROR_PROTOCOL_NOT_SUPPORTED;
	}

	assert (m_pTransportLayer != 0);
	return m_pTransportLayer->SetOptionAddMembership (rGroupAddress, m_hConnection);
}

int CSocket::SetOptionDropMembership (const CIPAddress &rGroupAddress)
{
	if (m_hConnection < 0)
	{
		return -NET_ERROR_NOT_CONNECTED;
	}

	if (m_nProtocol != IPPROTO_UDP)
	{
		return -NET_ERROR_PROTOCOL_NOT_SUPPORTED;
	}

	assert (m_pTransportLayer != 0);
	return m_pTransportLayer->SetOptionDropMembership (rGroupAddress, m_hConnection);
}

u16 CSocket::GetOwnPort (void) const
{
	if (m_hConnection < 0)
	{
		return m_nOwnPort;
	}

	assert (m_pTransportLayer != 0);
	return m_pTransportLayer->GetOwnPort (m_hConnection);
}

const u8 *CSocket::GetForeignIP (void) const
{
	if (m_hConnection < 0)
	{
		return 0;
	}

	assert (m_pTransportLayer != 0);
	return m_pTransportLayer->GetForeignIP (m_hConnection);
}

CSocket::TStatus CSocket::GetStatus (void) const
{
	TStatus Status = {FALSE, FALSE, FALSE, FALSE};

	assert (m_pTransportLayer != 0);

	if (m_nBackLog != 0)	// socket is listening
	{
		assert (m_nBackLog <= SOCKET_MAX_LISTEN_BACKLOG);

		for (unsigned i = 0; i < m_nBackLog; i++)
		{
			if (m_pTransportLayer->IsConnected (m_hListenConnection[i]))
			{
				Status.bRxReady = TRUE;

				break;
			}
		}

		return Status;
	}

	if (m_hConnection < 0)
	{
		return Status;
	}

	return m_pTransportLayer->GetStatus (m_hConnection);
}
