//
// netdevlayer.cpp
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
#include <circle/net/netdevlayer.h>
#include <circle/devicenameservice.h>
#include <circle/logger.h>
#include <assert.h>

const char FromNetDev[] = "netdev";

CNetDeviceLayer::CNetDeviceLayer (CNetConfig *pNetConfig)
:	m_pNetConfig (pNetConfig),
	m_pDevice (0),
	m_pBuffer (0)
{
}

CNetDeviceLayer::~CNetDeviceLayer (void)
{
	delete [] m_pBuffer;
	m_pBuffer = 0;

	m_pDevice = 0;
	m_pNetConfig = 0;
}

boolean CNetDeviceLayer::Initialize (void)
{
	assert (m_pDevice == 0);
	m_pDevice = (CNetDevice *) CDeviceNameService::Get ()->GetDevice ("eth0", FALSE);
	if (m_pDevice == 0)
	{
		CLogger::Get ()->Write (FromNetDev, LogError, "Net device not available");

		return FALSE;
	}

	assert (m_pBuffer == 0);
	m_pBuffer = new unsigned char[FRAME_BUFFER_SIZE];
	assert (m_pBuffer != 0);

	return TRUE;
}

void CNetDeviceLayer::Process (void)
{
	assert (m_pDevice != 0);
	assert (m_pBuffer != 0);

	unsigned nLength;
	while ((nLength = m_TxQueue.Dequeue (m_pBuffer)) > 0)
	{
		if (!m_pDevice->SendFrame (m_pBuffer, nLength))
		{
			CLogger::Get ()->Write (FromNetDev, LogWarning, "Frame dropped");

			break;
		}
	}

	while (m_pDevice->ReceiveFrame (m_pBuffer, &nLength))
	{
		assert (nLength > 0);
		m_RxQueue.Enqueue (m_pBuffer, nLength);
	}
}

const CMACAddress *CNetDeviceLayer::GetMACAddress (void) const
{
	assert (m_pDevice != 0);
	return m_pDevice->GetMACAddress ();
}

void CNetDeviceLayer::Send (const void *pBuffer, unsigned nLength)
{
	m_TxQueue.Enqueue (pBuffer, nLength);
}

boolean CNetDeviceLayer::Receive (void *pBuffer, unsigned *pResultLength)
{
	unsigned nLength = m_RxQueue.Dequeue (pBuffer);
	if (nLength == 0)
	{
		return FALSE;
	}

	assert (pResultLength != 0);
	*pResultLength = nLength;

	return TRUE;
}
