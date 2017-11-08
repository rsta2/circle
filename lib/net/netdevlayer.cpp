//
// netdevlayer.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2017  R. Stange <rsta2@o2online.de>
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
#include <circle/macros.h>
#include <assert.h>

const char FromNetDev[] = "netdev";

CNetDeviceLayer::CNetDeviceLayer (CNetConfig *pNetConfig)
:	m_pNetConfig (pNetConfig),
	m_pDevice (0)
{
}

CNetDeviceLayer::~CNetDeviceLayer (void)
{
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

	return TRUE;
}

void CNetDeviceLayer::Process (void)
{
	assert (m_pDevice != 0);

	u8 Buffer[FRAME_BUFFER_SIZE] ALIGN(4);		// DMA buffer
	unsigned nLength;
	while ((nLength = m_TxQueue.Dequeue (Buffer)) > 0)
	{
		if (!m_pDevice->SendFrame (Buffer, nLength))
		{
			CLogger::Get ()->Write (FromNetDev, LogWarning, "Frame dropped");

			break;
		}
	}

	while (m_pDevice->ReceiveFrame (Buffer, &nLength))
	{
		assert (nLength > 0);
		m_RxQueue.Enqueue (Buffer, nLength);
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
