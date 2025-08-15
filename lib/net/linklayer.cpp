//
// linklayer.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2025  R. Stange <rsta2gmx.net>
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
#include <circle/net/linklayer.h>
#include <circle/net/networklayer.h>
#include <circle/util.h>
#include <assert.h>

struct TRawPrivateData
{
	u8	MACSender[MAC_ADDRESS_SIZE];
};

CLinkLayer::CLinkLayer (CNetConfig *pNetConfig, CNetDeviceLayer *pNetDevLayer)
:	m_pNetConfig (pNetConfig),
	m_pNetDevLayer (pNetDevLayer),
	m_pNetworkLayer (0),
	m_pARPHandler (0),
	m_nRawProtocolType (0)
{
	assert (m_pNetConfig != 0);
	assert (m_pNetDevLayer != 0);

	for (unsigned i = 0; i < MaxGroups; i++)
	{
		m_nMulticastUseCounter[i] = 0;
	}
}

CLinkLayer::~CLinkLayer (void)
{
	delete m_pARPHandler;
	m_pARPHandler = 0;

	m_pNetworkLayer = 0;
	m_pNetDevLayer = 0;
	m_pNetConfig = 0;
}

boolean CLinkLayer::Initialize (void)
{
	assert (m_pNetConfig != 0);
	m_pARPHandler = new CARPHandler (m_pNetConfig, m_pNetDevLayer, this, &m_ARPRxQueue);
	assert (m_pARPHandler != 0);

	return TRUE;
}

void CLinkLayer::AttachLayer (CNetworkLayer *pNetworkLayer)
{
	assert (m_pNetworkLayer == 0);
	m_pNetworkLayer = pNetworkLayer;
	assert (m_pNetworkLayer != 0);
}

void CLinkLayer::Process (void)
{
	assert (m_pNetDevLayer != 0);
	const CMACAddress *pOwnMACAddress = m_pNetDevLayer->GetMACAddress ();
	if (pOwnMACAddress == 0)
	{
		return;
	}

	assert (m_pNetDevLayer != 0);
	u8 Buffer[FRAME_BUFFER_SIZE];
	unsigned nLength;
	while (m_pNetDevLayer->Receive (Buffer, &nLength))
	{
		assert (nLength <= FRAME_BUFFER_SIZE);
		if (nLength <= sizeof (TEthernetHeader))
		{
			continue;
		}
		TEthernetHeader *pHeader = (TEthernetHeader *) Buffer;

		CMACAddress MACAddressReceiver (pHeader->MACReceiver);
		if (    MACAddressReceiver != *pOwnMACAddress
		    && !MACAddressReceiver.IsBroadcast ())
		{
			if (!MACAddressReceiver.IsMulticast ())
			{
				continue;
			}

			unsigned i;
			for (i = 0; i < MaxGroups; i++)
			{
				if (   m_nMulticastUseCounter[i] > 0
				    && m_MulticastGroup[i] == MACAddressReceiver)
				{
					break;
				}
			}

			if (i == MaxGroups)
			{
				continue;
			}
		}

		nLength -= sizeof (TEthernetHeader);
		assert (nLength > 0);
		
		switch (pHeader->nProtocolType)
		{
		case BE (ETH_PROT_IP):
			m_IPRxQueue.Enqueue (Buffer+sizeof (TEthernetHeader), nLength);
			break;

		case BE (ETH_PROT_ARP):
			m_ARPRxQueue.Enqueue (Buffer+sizeof (TEthernetHeader), nLength);
			break;

		default:
			if (pHeader->nProtocolType == m_nRawProtocolType)
			{
				TRawPrivateData *pParam = new TRawPrivateData;
				assert (pParam != 0);
				memcpy (pParam->MACSender, pHeader->MACSender, MAC_ADDRESS_SIZE);

				m_RawRxQueue.Enqueue (Buffer+sizeof (TEthernetHeader),
						      nLength, pParam);
			}
			break;
		}
	}

	assert (m_pARPHandler != 0);
	m_pARPHandler->Process ();
}

boolean CLinkLayer::Send (const CIPAddress &rReceiver, const void *pIPPacket, unsigned nLength)
{
	unsigned nFrameLength = sizeof (TEthernetHeader) + nLength;	// may wrap
	if (   nFrameLength <= sizeof (TEthernetHeader)
	    || nFrameLength > FRAME_BUFFER_SIZE)
	{
		return FALSE;
	}

	assert (pIPPacket != 0);
	assert (nLength > 0);
	assert (m_pNetConfig != 0);
	if (   !rReceiver.IsNull ()
	    && rReceiver == *m_pNetConfig->GetIPAddress ())
	{
		m_IPRxQueue.Enqueue (pIPPacket, nLength);	// loop back to own address

		return TRUE;
	}

	u8 FrameBuffer[nFrameLength];
	TEthernetHeader *pHeader = (TEthernetHeader *) FrameBuffer;

	assert (m_pNetDevLayer != 0);
	const CMACAddress *pOwnMACAddress = m_pNetDevLayer->GetMACAddress ();
	assert (pOwnMACAddress != 0);
	pOwnMACAddress->CopyTo (pHeader->MACSender);

	pHeader->nProtocolType = BE (ETH_PROT_IP);

	memcpy (FrameBuffer+sizeof (TEthernetHeader), pIPPacket, nLength);

	assert (m_pARPHandler != 0);
	CMACAddress MACAddressReceiver;
	if (   rReceiver.IsBroadcast ()
	    || rReceiver == *m_pNetConfig->GetBroadcastAddress ())
	{
		MACAddressReceiver.SetBroadcast ();
	}
	else if (rReceiver.IsMulticast ())
	{
		MACAddressReceiver.SetMulticast (rReceiver.Get ());
	}
	else if (!m_pARPHandler->Resolve (rReceiver, &MACAddressReceiver,
					  FrameBuffer, nFrameLength))
	{
		return TRUE;		// packet will be retransmitted by ARP handler
	}

	MACAddressReceiver.CopyTo (pHeader->MACReceiver);

	m_pNetDevLayer->Send (FrameBuffer, nFrameLength);

	return TRUE;
}

boolean CLinkLayer::Receive (void *pBuffer, unsigned *pResultLength)
{
	assert (pBuffer != 0);
	assert (pResultLength != 0);
	*pResultLength = m_IPRxQueue.Dequeue (pBuffer);

	return *pResultLength != 0 ? TRUE : FALSE;
}

boolean CLinkLayer::SendRaw (const void *pFrame, unsigned nLength)
{
	assert (pFrame != 0);
	assert (nLength > 0);
	assert (m_pNetDevLayer != 0);
	m_pNetDevLayer->Send (pFrame, nLength);

	return TRUE;
}

boolean CLinkLayer::ReceiveRaw (void *pBuffer, unsigned *pResultLength, CMACAddress *pSender)
{
	void *pParam;
	assert (pBuffer != 0);
	assert (pResultLength != 0);
	*pResultLength = m_RawRxQueue.Dequeue (pBuffer, &pParam);
	if (*pResultLength == 0)
	{
		return FALSE;
	}

	TRawPrivateData *pData = (TRawPrivateData *) pParam;

	if (pSender != 0)
	{
		assert (pData != 0);
		pSender->Set (pData->MACSender);
	}

	delete pData;

	return TRUE;
}

boolean CLinkLayer::EnableReceiveRaw (u16 nProtocolType)
{
	if (m_nRawProtocolType != 0)
	{
		return FALSE;
	}

	assert (nProtocolType != 0);
	m_nRawProtocolType = le2be16 (nProtocolType);

	return TRUE;
}

boolean CLinkLayer::IsRunning (void) const
{
	assert (m_pNetDevLayer != 0);
	return m_pNetDevLayer->IsRunning ();
}

boolean CLinkLayer::JoinLocalGroup (const CIPAddress &rGroupAddress)
{
	CMACAddress Group;
	Group.SetMulticast (rGroupAddress.Get ());

	unsigned j = MaxGroups;
	for (unsigned i = 0; i < MaxGroups; i++)
	{
		if (m_nMulticastUseCounter[i] == 0)
		{
			if (j == MaxGroups)
			{
				j = i;
			}

			continue;
		}

		if (m_MulticastGroup[i] == Group)
		{
			m_nMulticastUseCounter[i]++;

			return TRUE;
		}
	}

	if (j == MaxGroups)
	{
		return FALSE;
	}

	m_MulticastGroup[j].Set (Group.Get ());
	m_nMulticastUseCounter[j]++;

	return UpdateMulticastFilter ();
}

boolean CLinkLayer::LeaveLocalGroup (const CIPAddress &rGroupAddress)
{
	CMACAddress Group;
	Group.SetMulticast (rGroupAddress.Get ());

	for (unsigned i = 0; i < MaxGroups; i++)
	{
		if (   m_nMulticastUseCounter[i] > 0
		    && m_MulticastGroup[i] == Group)
		{
			if (--m_nMulticastUseCounter[i] > 0)
			{
				return TRUE;
			}

			return UpdateMulticastFilter ();
		}
	}

	return FALSE;
}

boolean CLinkLayer::UpdateMulticastFilter (void)
{
	u8 Groups[MaxGroups+1][MAC_ADDRESS_SIZE];
	memset (Groups, 0, sizeof Groups);

	for (unsigned i = 0; i < MaxGroups; i++)
	{
		if (m_nMulticastUseCounter[i] > 0)
		{
			m_MulticastGroup[i].CopyTo (Groups[i]);
		}
	}

	assert (m_pNetDevLayer != 0);
	return m_pNetDevLayer->SetMulticastFilter (Groups);
}

void CLinkLayer::ResolveFailed (const void *pReturnedFrame, unsigned nLength)
{
	assert (pReturnedFrame != 0);
	assert (nLength > sizeof (TEthernetHeader));
	assert (m_pNetworkLayer != 0);

	m_pNetworkLayer->SendFailed (ICMP_CODE_DEST_HOST_UNREACH,
				     (const u8 *) pReturnedFrame + sizeof (TEthernetHeader),
				     nLength - sizeof (TEthernetHeader));
}
