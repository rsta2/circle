//
// transportlayer.cpp
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
#include <circle/net/transportlayer.h>
//#include <circle/net/tcpconnection.h>
#include <circle/net/udpconnection.h>
#include <circle/net/in.h>
#include <circle/macros.h>
#include <assert.h>

#define OWN_PORT_MIN	60000
#define OWN_PORT_MAX	60999

CTransportLayer::CTransportLayer (CNetConfig *pNetConfig, CNetworkLayer *pNetworkLayer)
:	m_pNetConfig (pNetConfig),
	m_pNetworkLayer (pNetworkLayer),
	m_nOwnPort (OWN_PORT_MIN),
	m_SpinLock (FALSE),
	m_pBuffer (0)
{
	assert (m_pNetConfig != 0);
	assert (m_pNetworkLayer != 0);

	for (unsigned i = 0; i < MAX_SOCKETS; i++)
	{
		m_pConnection[i] = 0;
	}
}

CTransportLayer::~CTransportLayer (void)
{
#ifndef NDEBUG
	for (unsigned i = 0; i < MAX_SOCKETS; i++)
	{
		assert (m_pConnection[i] == 0);
	}
#endif

	delete [] m_pBuffer;
	m_pBuffer = 0;

	m_pNetworkLayer = 0;
	m_pNetConfig = 0;
}

boolean CTransportLayer::Initialize (void)
{
	assert (m_pBuffer == 0);
	m_pBuffer = new u8[FRAME_BUFFER_SIZE];
	assert (m_pBuffer != 0);

	return TRUE;
}

void CTransportLayer::Process (void)
{
	unsigned nResultLength;
	CIPAddress Sender;
	int nProtocol;
	assert (m_pNetworkLayer != 0);
	assert (m_pBuffer != 0);
	while (m_pNetworkLayer->Receive (m_pBuffer, &nResultLength, &Sender, &nProtocol))
	{
		for (unsigned i = 0; i < MAX_SOCKETS; i++)
		{
			if (m_pConnection[i] == 0)
			{
				continue;
			}

			if (m_pConnection[i]->PacketReceived (m_pBuffer, nResultLength, Sender, nProtocol) != 0)
			{
				break;
			}
		}

		// TODO: send RST on not consumed TCP segment
	}
	
	for (unsigned i = 0; i < MAX_SOCKETS; i++)
	{
		if (m_pConnection[i] != 0)
		{
			if (!m_pConnection[i]->IsTerminated ())
			{			
				m_pConnection[i]->Process ();
			}
			else
			{
				delete m_pConnection[i];
				m_pConnection[i] = 0;
			}
		}
	}
}
	
int CTransportLayer::Connect (CIPAddress &rIPAddress, u16 nPort, u16 nOwnPort, int nProtocol)
{
	m_SpinLock.Acquire ();

	unsigned i;
	for (i = 0; i < MAX_SOCKETS; i++)
	{
		if (m_pConnection[i] == 0)
		{
			break;
		}
	}

	if (i >= MAX_SOCKETS)
	{
		m_SpinLock.Release ();

		return -1;
	}

	if (nOwnPort == 0)
	{
		unsigned j;
		do
		{
			nOwnPort = m_nOwnPort;
			if (++m_nOwnPort > OWN_PORT_MAX)
			{
				m_nOwnPort = OWN_PORT_MIN;
			}

			for (j = 0; j < MAX_SOCKETS; j++)
			{
				if (   m_pConnection[j] != 0
				    && m_pConnection[j]->GetOwnPort () == nOwnPort
				    && m_pConnection[j]->GetProtocol () == nProtocol)
				{
					break;
				}
			}
		}
		while (j < MAX_SOCKETS);
	}

	assert (m_pNetConfig != 0);
	assert (m_pNetworkLayer != 0);
	switch (nProtocol)
	{
#if 0
	case IPPROTO_TCP:
		m_pConnection[i] = new CTCPConnection (m_pNetConfig, m_pNetworkLayer, rIPAddress, nPort, nOwnPort);
		break;
#endif

	case IPPROTO_UDP:
		m_pConnection[i] = new CUDPConnection (m_pNetConfig, m_pNetworkLayer, rIPAddress, nPort, nOwnPort);
		break;

	default:
		m_SpinLock.Release ();
		return -1;
	}
	assert (m_pConnection[i] != 0);

	int nResult = m_pConnection[i]->Connect ();
	if (nResult < 0)
	{
		m_SpinLock.Release ();

		return -1;
	}
	
	m_SpinLock.Release ();

	return i;
}

int CTransportLayer::Listen (u16 nOwnPort, int nProtocol)
{
#if 0
	m_SpinLock.Acquire ();

	unsigned i;
	for (i = 0; i < MAX_SOCKETS; i++)
	{
		if (m_pConnection[i] == 0)
		{
			break;
		}
	}

	if (i >= MAX_SOCKETS)
	{
		m_SpinLock.Release ();

		return -1;
	}

	if (nOwnPort == 0)
	{
		m_SpinLock.Release ();

		return -1;
	}

	if (nProtocol != IPPROTO_TCP)
	{
		m_SpinLock.Release ();

		return -1;
	}

	assert (m_pNetConfig != 0);
	assert (m_pNetworkLayer != 0);
	m_pConnection[i] = new CTCPConnection (m_pNetConfig, m_pNetworkLayer, nOwnPort);
	assert (m_pConnection[i] != 0);

	m_SpinLock.Release ();

	return i;
#else
	return -1;
#endif
}

int CTransportLayer::Accept (CIPAddress *pForeignIP, u16 *pForeignPort, int hConnection)
{
	assert (hConnection >= 0);
	assert (hConnection < MAX_SOCKETS);

	if (m_pConnection[hConnection] == 0)
	{
		return -1;
	}

	assert (pForeignIP != 0);
	assert (pForeignPort != 0);
	return m_pConnection[hConnection]->Accept (pForeignIP, pForeignPort);
}

int CTransportLayer::Disconnect (int hConnection)
{
	assert (hConnection >= 0);
	assert (hConnection < MAX_SOCKETS);

	if (m_pConnection[hConnection] == 0)
	{
		return -1;
	}

	return m_pConnection[hConnection]->Close ();
}

int CTransportLayer::Send (const void *pData, unsigned nLength, int nFlags, int hConnection)
{
	assert (hConnection >= 0);
	assert (hConnection < MAX_SOCKETS);

	if (m_pConnection[hConnection] == 0)
	{
		return -1;
	}

	assert (pData != 0);
	assert (nLength > 0);
	return m_pConnection[hConnection]->Send (pData, nLength, nFlags);
}

int CTransportLayer::Receive (void *pBuffer, int nFlags, int hConnection)
{
	assert (hConnection >= 0);
	assert (hConnection < MAX_SOCKETS);

	if (m_pConnection[hConnection] == 0)
	{
		return -1;
	}

	assert (pBuffer != 0);
	return m_pConnection[hConnection]->Receive (pBuffer, nFlags);
}
