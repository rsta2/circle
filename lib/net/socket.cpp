//
// socket.cpp
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
#include <circle/net/socket.h>
#include <circle/net/netsubsystem.h>
#include <circle/net/in.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <assert.h>

static const char FromSocket[] = "socket";

CSocket::CSocket (CNetSubSystem *pNetSubSystem, int nProtocol)
:	m_pNetConfig (pNetSubSystem->GetConfig ()),
	m_pTransportLayer (pNetSubSystem->GetTransportLayer ()),
	m_nProtocol (nProtocol),
	m_nOwnPort (0),
	m_hConnection (-1),
	m_pBuffer (0)
{
	assert (m_pNetConfig != 0);
	assert (m_pTransportLayer != 0);

	m_pBuffer = new u8[FRAME_BUFFER_SIZE];
	assert (m_pBuffer != 0);
}

CSocket::CSocket (CSocket &rSocket)
:	m_pNetConfig (rSocket.m_pNetConfig),
	m_pTransportLayer (rSocket.m_pTransportLayer),
	m_nProtocol (rSocket.m_nProtocol),
	m_nOwnPort (rSocket.m_nOwnPort),
	m_hConnection (rSocket.m_hConnection),
	m_pBuffer (0)
{
	assert (m_pNetConfig != 0);
	assert (m_pTransportLayer != 0);

	m_pBuffer = new u8[FRAME_BUFFER_SIZE];
	assert (m_pBuffer != 0);
}

CSocket::~CSocket (void)
{
	if (m_hConnection >= 0)
	{
		assert (m_pTransportLayer != 0);
		m_pTransportLayer->Disconnect (m_hConnection);
		m_hConnection = -1;
	}

	delete m_pBuffer;
	m_pBuffer = 0;

	m_pTransportLayer = 0;
	m_pNetConfig = 0;
}

int CSocket::Bind (u16 nOwnPort)
{
	if (nOwnPort == 0)
	{
		return -1;
	}
	
	if (m_nOwnPort != 0)
	{
		return -1;
	}

	if (m_hConnection >= 0)
	{
		return -1;
	}
	
	m_nOwnPort = nOwnPort;				// TODO: suppress double usage of port
	
	if (m_nProtocol == IPPROTO_UDP)
	{
		m_hConnection = m_pTransportLayer->Bind (m_nOwnPort, m_nProtocol);
		if (m_hConnection < 0)
		{
			return m_hConnection;		// return error code
		}
	}

	return 0;
}

int CSocket::Connect (CIPAddress &rForeignIP, u16 nForeignPort)
{
	if (nForeignPort == 0)
	{
		return -1;
	}

	assert (m_pTransportLayer != 0);

	if (m_hConnection >= 0)
	{
		if (m_nProtocol != IPPROTO_UDP)
		{
			return -1;
		}

		m_pTransportLayer->Disconnect (m_hConnection);
		m_hConnection = -1;
	}

	assert (m_pNetConfig != 0);
	if (   m_pNetConfig->GetIPAddress ()->IsNull ()		// from null source address
	    && !(   m_nProtocol == IPPROTO_UDP			// only UDP broadcasts are allowed
		 && rForeignIP.IsBroadcast ()))
	{
		return -1;
	}

	m_hConnection = m_pTransportLayer->Connect (rForeignIP, nForeignPort, m_nOwnPort, m_nProtocol);

	return m_hConnection >= 0 ? 0 : m_hConnection;
}

int CSocket::Listen (void)
{
	if (   m_nProtocol != IPPROTO_TCP
	    || m_nOwnPort == 0)
	{
		return -1;
	}

	if (m_hConnection >= 0)
	{
		return -1;
	}

	assert (m_pTransportLayer != 0);
	m_hConnection = m_pTransportLayer->Listen (m_nOwnPort, m_nProtocol);

	return m_hConnection >= 0 ? 0 : m_hConnection;
}

CSocket *CSocket::Accept (CIPAddress *pForeignIP, u16 *pForeignPort)
{
	if (   m_hConnection < 0
	    || m_nOwnPort == 0)
	{
		return 0;
	}

	CSocket *pNewSocket = 0;

	assert (pForeignIP != 0);
	assert (pForeignPort != 0);
	assert (m_pTransportLayer != 0);
	if (m_pTransportLayer->Accept (pForeignIP, pForeignPort, m_hConnection) >= 0)
	{
		pNewSocket = new CSocket (*this);
		assert (pNewSocket != 0);
	}

	// if this fails no further connection can be accepted
	m_hConnection = m_pTransportLayer->Listen (m_nOwnPort, m_nProtocol);
	if (m_hConnection < 0)
	{
		CLogger::Get ()->Write (FromSocket, LogPanic, "Re-Listen failed. Limit number of clients!");
	}

	return pNewSocket;
}

int CSocket::Send (const void *pBuffer, unsigned nLength, int nFlags)
{
	if (m_hConnection < 0)
	{
		return -1;
	}

	if (nLength == 0)
	{
		return -1;
	}
	
	assert (m_pTransportLayer != 0);
	assert (pBuffer != 0);
	return m_pTransportLayer->Send (pBuffer, nLength, nFlags, m_hConnection);
}

int CSocket::Receive (void *pBuffer, unsigned nLength, int nFlags)
{
	if (m_hConnection < 0)
	{
		return -1;
	}
	
	if (nLength == 0)
	{
		return -1;
	}
	
	assert (m_pTransportLayer != 0);
	assert (m_pBuffer != 0);
	int nResult = m_pTransportLayer->Receive (m_pBuffer, nFlags, m_hConnection);
	if (nResult < 0)
	{
		return nResult;
	}

	if (nLength < (unsigned) nResult)
	{
		nResult = nLength;
	}

	assert (pBuffer != 0);
	memcpy (pBuffer, m_pBuffer, nResult);

	return nResult;
}

int CSocket::SendTo (const void *pBuffer, unsigned nLength, int nFlags,
		     CIPAddress &rForeignIP, u16 nForeignPort)
{
	if (m_hConnection < 0)
	{
		return -1;
	}

	if (nLength == 0)
	{
		return -1;
	}
	
	assert (m_pNetConfig != 0);
	if (m_pNetConfig->GetIPAddress ()->IsNull ())		// from null source address
	{
		return -1;
	}

	if (nForeignPort == 0)
	{
		return -1;
	}

	assert (m_pTransportLayer != 0);
	assert (pBuffer != 0);
	return m_pTransportLayer->SendTo (pBuffer, nLength, nFlags, rForeignIP, nForeignPort, m_hConnection);
}

int CSocket::ReceiveFrom (void *pBuffer, unsigned nLength, int nFlags,
			  CIPAddress *pForeignIP, u16 *pForeignPort)
{
	if (m_hConnection < 0)
	{
		return -1;
	}
	
	if (nLength == 0)
	{
		return -1;
	}
	
	assert (m_pTransportLayer != 0);
	assert (m_pBuffer != 0);
	int nResult = m_pTransportLayer->ReceiveFrom (m_pBuffer, nFlags,
						      pForeignIP, pForeignPort, m_hConnection);
	if (nResult < 0)
	{
		return nResult;
	}

	if (nLength < (unsigned) nResult)
	{
		nResult = nLength;
	}

	assert (pBuffer != 0);
	memcpy (pBuffer, m_pBuffer, nResult);

	return nResult;
}

int CSocket::SetOptionBroadcast (boolean bAllowed)
{
	if (m_hConnection < 0)
	{
		return -1;
	}

	if (m_nProtocol != IPPROTO_UDP)
	{
		return 0;
	}

	assert (m_pTransportLayer != 0);
	return m_pTransportLayer->SetOptionBroadcast (bAllowed, m_hConnection);
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
