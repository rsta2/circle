//
// netdevlayer.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2025  R. Stange <rsta2@gmx.net>
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
#include <circle/net/phytask.h>
#include <circle/logger.h>
#include <circle/timer.h>
#include <circle/synchronize.h>
#include <circle/macros.h>
#include <circle/util.h>
#include <assert.h>

const char FromNetDev[] = "netdev";

CNetDeviceLayer::CNetDeviceLayer (CNetConfig *pNetConfig, TNetDeviceType DeviceType)
:	m_DeviceType (DeviceType),
	m_pNetConfig (pNetConfig),
	m_pDevice (0),
	m_pRxBuffer (new CNetBuffer (CNetBuffer::Receive, FRAME_BUFFER_SIZE))
{
}

CNetDeviceLayer::~CNetDeviceLayer (void)
{
	delete m_pRxBuffer;
	m_pRxBuffer = 0;

	m_pDevice = 0;
	m_pNetConfig = 0;
}

boolean CNetDeviceLayer::Initialize (boolean bWaitForActivate)
{
#if RASPPI == 4
	if (!m_Bcm54213.Initialize ())
	{
		return FALSE;
	}
#elif RASPPI >= 5
	if (!m_MACB.Initialize ())
	{
		return FALSE;
	}
#endif

	if (!bWaitForActivate)
	{
		return TRUE;
	}

	assert (m_pDevice == 0);
	m_pDevice = CNetDevice::GetNetDevice (m_DeviceType);
	if (m_pDevice == 0)
	{
		CLogger::Get ()->Write (FromNetDev, LogError, "Net device not available");

		return FALSE;
	}

	new CPHYTask (m_pDevice);

	// wait for Ethernet PHY to come up
	unsigned nStartTicks = CTimer::Get ()->GetTicks ();
	do
	{
		if (CTimer::Get ()->GetTicks () - nStartTicks >= 4*HZ)
		{
			CLogger::Get ()->Write (FromNetDev, LogWarning, "Link is down");

			return TRUE;
		}
	}
	while (!m_pDevice->IsLinkUp ());

	TNetDeviceSpeed Speed = m_pDevice->GetLinkSpeed ();
	if (Speed != NetDeviceSpeedUnknown)
	{
		CLogger::Get ()->Write (FromNetDev, LogNotice, "Link is %s",
					CNetDevice::GetSpeedString (Speed));
	}

	return TRUE;
}

void CNetDeviceLayer::Process (void)
{
	if (m_pDevice == 0)
	{
		m_pDevice = CNetDevice::GetNetDevice (m_DeviceType);
		if (m_pDevice == 0)
		{
			return;
		}

		new CPHYTask (m_pDevice);
	}

	DMA_BUFFER (u8, Buffer, FRAME_BUFFER_SIZE);
	unsigned nLength;
	CNetBuffer *pTxBuffer;
	while (   m_pDevice->IsSendFrameAdvisable ()
	       && (pTxBuffer = m_TxQueue.Dequeue ()) != 0)
	{
		void *pBuffer = pTxBuffer->GetPtr ();
		nLength = pTxBuffer->GetLength ();
		assert (pBuffer != 0);
		assert (nLength != 0);

		if (unlikely (!IS_CACHE_ALIGNED (pBuffer, 0)))
		{
			memcpy (Buffer, pBuffer, nLength);
			pBuffer = Buffer;

			static boolean bShowOnce = FALSE;
			if (!bShowOnce)
			{
				CLogger::Get ()->Write (FromNetDev, LogWarning,
							"Buffer is not cache aligned");

				pTxBuffer->Dump (FromNetDev);

				bShowOnce = TRUE;
			}
		}

		if (!m_pDevice->SendFrame (pBuffer, nLength))
		{
			CLogger::Get ()->Write (FromNetDev, LogWarning, "Frame dropped");

			delete pTxBuffer;

			break;
		}

		delete pTxBuffer;
	}

	assert (m_pRxBuffer != 0);
	while (m_pDevice->ReceiveFrame (m_pRxBuffer->GetPtr (), &nLength))
	{
		assert (nLength < FRAME_BUFFER_SIZE);
		m_pRxBuffer->RemoveTrailer (FRAME_BUFFER_SIZE - nLength);

		m_RxQueue.Enqueue (m_pRxBuffer);

		m_pRxBuffer = new CNetBuffer (CNetBuffer::Receive, FRAME_BUFFER_SIZE);
		assert (m_pRxBuffer != 0);
	}
}

const CMACAddress *CNetDeviceLayer::GetMACAddress (void) const
{
	if (m_pDevice == 0)
	{
		return 0;
	}

	return m_pDevice->GetMACAddress ();
}

void CNetDeviceLayer::Send (CNetBuffer *pNetBuffer)
{
	m_TxQueue.Enqueue (pNetBuffer);
}

CNetBuffer *CNetDeviceLayer::Receive (void)
{
	return m_RxQueue.Dequeue ();
}

boolean CNetDeviceLayer::IsRunning (void) const
{
	return m_pDevice != 0 && m_pDevice->IsLinkUp ();
}

boolean CNetDeviceLayer::SetMulticastFilter (const u8 Groups[][MAC_ADDRESS_SIZE])
{
	assert (m_pDevice != 0);
	return m_pDevice->SetMulticastFilter (Groups);
}
