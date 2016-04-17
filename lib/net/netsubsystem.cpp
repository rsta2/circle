//
// netsubsystem.cpp
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
#include <circle/net/netsubsystem.h>
#include <circle/net/nettask.h>
#include <circle/net/dhcpclient.h>
#include <circle/sched/scheduler.h>
#include <circle/timer.h>
#include <assert.h>

CNetSubSystem::CNetSubSystem (const u8 *pIPAddress, const u8 *pNetMask, const u8 *pDefaultGateway, const u8 *pDNSServer)
:	m_NetDevLayer (&m_Config),
	m_LinkLayer (&m_Config, &m_NetDevLayer),
	m_NetworkLayer (&m_Config, &m_LinkLayer),
	m_TransportLayer (&m_Config, &m_NetworkLayer),
	m_bUseDHCP (pIPAddress == 0 ? TRUE : FALSE),
	m_pDHCPClient (0)
{
	m_Config.SetDHCP (m_bUseDHCP);

	if (!m_bUseDHCP)
	{
		m_Config.SetIPAddress (pIPAddress);
		m_Config.SetNetMask (pNetMask);

		if (pDefaultGateway != 0)
		{
			m_Config.SetDefaultGateway (pDefaultGateway);
		}

		if (pDNSServer != 0)
		{
			m_Config.SetDNSServer (pDNSServer);
		}
	}
}

CNetSubSystem::~CNetSubSystem (void)
{
	delete m_pDHCPClient;
	m_pDHCPClient = 0;
}

boolean CNetSubSystem::Initialize (void)
{
	m_bUseDHCP = m_Config.GetIPAddress ()->IsNull ();
	m_Config.SetDHCP (m_bUseDHCP);

	// wait for Ethernet PHY to come up
	CTimer::Get ()->MsDelay (2000);

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

	new CNetTask (this);

	if (m_bUseDHCP)
	{
		assert (m_pDHCPClient == 0);
		m_pDHCPClient = new CDHCPClient (this);
		assert (m_pDHCPClient != 0);
	}

	while (!IsRunning ())
	{
		CScheduler::Get ()->Yield ();
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

CNetConfig *CNetSubSystem::GetConfig (void)
{
	return &m_Config;
}

CNetDeviceLayer *CNetSubSystem::GetNetDeviceLayer (void)
{
	return &m_NetDevLayer;
}

CTransportLayer *CNetSubSystem::GetTransportLayer (void)
{
	return &m_TransportLayer;
}

boolean CNetSubSystem::IsRunning (void) const
{
	if (!m_bUseDHCP)
	{
		return TRUE;
	}

	assert (m_pDHCPClient != 0);
	return m_pDHCPClient->IsBound ();
}
