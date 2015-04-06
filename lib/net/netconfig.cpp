//
// netconfig.cpp
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
#include <circle/net/netconfig.h>

static const u8 IPAddress[]      = {192, 168, 0, 250};
static const u8 NetMask[]        = {255, 255, 255, 0};
static const u8 DefaultGateway[] = {192, 168, 0, 1};
static const u8 DNSServer[]      = {192, 168, 0, 1};

CNetConfig::CNetConfig (void)
:	m_IPAddress (IPAddress),
	m_NetMask (NetMask),
	m_DefaultGateway (DefaultGateway),
	m_DNSServer (DNSServer)
{
}

CNetConfig::~CNetConfig (void)
{
}

void CNetConfig::SetIPAddress (u32 nAddress)
{
	m_IPAddress.Set (nAddress);
}

void CNetConfig::SetNetMask (u32 nNetMask)
{
	m_NetMask.Set (nNetMask);
}

void CNetConfig::SetDefaultGateway (u32 nAddress)
{
	m_DefaultGateway.Set (nAddress);
}

void CNetConfig::SetDNSServer (u32 nAddress)
{
	m_DNSServer.Set (nAddress);
}

void CNetConfig::SetIPAddress (const u8 *pAddress)
{
	m_IPAddress.Set (pAddress);
}

void CNetConfig::SetNetMask (const u8 *pNetMask)
{
	m_NetMask.Set (pNetMask);
}

void CNetConfig::SetDefaultGateway (const u8 *pAddress)
{
	m_DefaultGateway.Set (pAddress);
}

void CNetConfig::SetDNSServer (const u8 *pAddress)
{
	m_DNSServer.Set (pAddress);
}

const CIPAddress *CNetConfig::GetIPAddress (void) const
{
	return &m_IPAddress;
}

const u8 *CNetConfig::GetNetMask (void) const
{
	return m_NetMask.Get ();
}

const CIPAddress *CNetConfig::GetDefaultGateway (void) const
{
	return &m_DefaultGateway;
}

const CIPAddress *CNetConfig::GetDNSServer (void) const
{
	return &m_DNSServer;
}
