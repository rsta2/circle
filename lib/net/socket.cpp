//
// socket.cpp
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
#include <circle/net/socket.h>
#include <circle/net/netsubsystem.h>
#include <circle/net/in.h>
#include <circle/util.h>
#include <assert.h>

CSocket::CSocket (CNetSubSystem *pNetSubSystem, int nProtocol)
:	m_pNetConfig (pNetSubSystem->GetConfig ()),
	m_pTransportLayer (pNetSubSystem->GetTransportLayer ()),
	m_nProtocol (nProtocol),
	m_hConnection (-1),
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

	m_hConnection = m_pTransportLayer->Connect (rForeignIP, nForeignPort, 0, m_nProtocol);

	return m_hConnection >= 0 ? 0 : m_hConnection;
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
