//
// netsubsystem.cpp
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
#include <circle/net/netsubsystem.h>
#include <circle/timer.h>
#include <assert.h>

CNetSubSystem::CNetSubSystem (const u8 *pIPAddress, const u8 *pNetMask, const u8 *pDefaultGateway, const u8 *pDNSServer)
:	m_NetDevLayer (&m_Config),
	m_LinkLayer (&m_Config, &m_NetDevLayer),
	m_NetworkLayer (&m_Config, &m_LinkLayer),
	m_TransportLayer (&m_Config, &m_NetworkLayer)
{
	m_Config.SetIPAddress (pIPAddress);
	m_Config.SetNetMask (pNetMask);
	m_Config.SetDefaultGateway (pDefaultGateway);
	m_Config.SetDNSServer (pDNSServer);
}

CNetSubSystem::~CNetSubSystem (void)
{
}

boolean CNetSubSystem::Initialize (void)
{
	if (!m_NetDevLayer.Initialize ())
	{
		return FALSE;
	}

	if (!m_LinkLayer.Initialize ())
	{
		return FALSE;
	}

	if (!m_NetworkLayer.Initialize ())
	{
		return FALSE;
	}

	if (!m_TransportLayer.Initialize ())
	{
		return FALSE;
	}

	return TRUE;
}

void CNetSubSystem::Process (void)
{
	m_NetDevLayer.Process ();

	m_LinkLayer.Process ();

	m_NetworkLayer.Process ();

	m_TransportLayer.Process ();
}

void CNetSubSystem::ProcessAndDelay (unsigned nHZDelay)
{
	CTimer *pTimer = CTimer::Get ();
	assert (pTimer != 0);

	unsigned nStartTicks = pTimer->GetTicks ();

	do
	{
		Process ();

		pTimer->usDelay (1000);
	}
	while (pTimer->GetTicks () - nStartTicks < nHZDelay);
}

CNetConfig *CNetSubSystem::GetConfig (void)
{
	return &m_Config;
}

CTransportLayer *CNetSubSystem::GetTransportLayer (void)
{
	return &m_TransportLayer;
}
