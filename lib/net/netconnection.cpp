//
// netconnection.cpp
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
#include <circle/net/netconnection.h>
#include <assert.h>

CNetConnection::CNetConnection (CNetConfig	*pNetConfig,
				CNetworkLayer	*pNetworkLayer,
				CIPAddress	&rForeignIP,
				u16		 nForeignPort,
				u16		 nOwnPort,
				int		 nProtocol)
:	m_pNetConfig (pNetConfig),
	m_pNetworkLayer (pNetworkLayer),
	m_ForeignIP (rForeignIP),
	m_nForeignPort (nForeignPort),
	m_nOwnPort (nOwnPort),
	m_nProtocol (nProtocol),
	m_Checksum (*pNetConfig->GetIPAddress (), rForeignIP, nProtocol)
{
	assert (m_pNetConfig != 0);
	assert (m_pNetworkLayer != 0);
}

CNetConnection::CNetConnection (CNetConfig	*pNetConfig,
				CNetworkLayer	*pNetworkLayer,
				u16		 nOwnPort,
				int		 nProtocol)
:	m_pNetConfig (pNetConfig),
	m_pNetworkLayer (pNetworkLayer),
	m_nForeignPort (0),
	m_nOwnPort (nOwnPort),
	m_Checksum (*pNetConfig->GetIPAddress (), nProtocol)
{
	assert (m_pNetConfig != 0);
	assert (m_pNetworkLayer != 0);
}

CNetConnection::~CNetConnection (void)
{
	m_pNetworkLayer = 0;
	m_pNetConfig = 0;
}

const u8 *CNetConnection::GetForeignIP (void) const
{
	return m_ForeignIP.Get ();
}

u16 CNetConnection::GetOwnPort (void) const
{
	assert (m_nOwnPort != 0);
	return m_nOwnPort;
}

int CNetConnection::GetProtocol (void) const
{
	return m_nProtocol;
}
