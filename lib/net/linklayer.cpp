//
// linklayer.cpp
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
#include <circle/net/linklayer.h>
#include <circle/net/networklayer.h>
#include <circle/net/netbuffer.h>
#include <circle/util.h>
#include <assert.h>

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
	CNetBuffer *pNetBuffer;
	while ((pNetBuffer = m_pNetDevLayer->Receive ()) != 0)
	{
		size_t nLength = pNetBuffer->GetLength ();
		assert (nLength <= FRAME_BUFFER_SIZE);
		if (nLength <= sizeof (TEthernetHeader))
		{
			delete pNetBuffer;

			continue;
		}

		TEthernetHeader *pHeader = (TEthernetHeader *) pNetBuffer->GetPtr ();
		assert (pHeader != 0);

		CMACAddress MACAddressReceiver (pHeader->MACReceiver);
		if (    MACAddressReceiver != *pOwnMACAddress
		    && !MACAddressReceiver.IsBroadcast ())
		{
			if (!MACAddressReceiver.IsMulticast ())
			{
				delete pNetBuffer;

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
				delete pNetBuffer;

				continue;
			}
		}

		pNetBuffer->RemoveHeader (sizeof (TEthernetHeader));

		switch (pHeader->nProtocolType)
		{
		case BE (ETH_PROT_IP):
			m_IPRxQueue.Enqueue (pNetBuffer);
			break;

		case BE (ETH_PROT_ARP):
			m_ARPRxQueue.Enqueue (pNetBuffer);
			break;

		default:
			if (pHeader->nProtocolType == m_nRawProtocolType)
			{
				pNetBuffer->SetPrivateData (pHeader->MACSender, MAC_ADDRESS_SIZE);

				m_RawRxQueue.Enqueue (pNetBuffer);
			}
			else
			{
				delete pNetBuffer;
			}
			break;
		}
	}

	assert (m_pARPHandler != 0);
	m_pARPHandler->Process ();
}

boolean CLinkLayer::Send (const CIPAddress &rReceiver, CNetBuffer *pNetBuffer)
{
	assert (pNetBuffer != 0);
	unsigned nFrameLength = sizeof (TEthernetHeader) + pNetBuffer->GetLength ();	// may wrap
	if (   nFrameLength <= sizeof (TEthernetHeader)
	    || nFrameLength > FRAME_BUFFER_SIZE)
	{
		return FALSE;
	}

	assert (m_pNetConfig != 0);
	if (   !rReceiver.IsNull ()
	    && rReceiver == *m_pNetConfig->GetIPAddress ())
	{
		m_IPRxQueue.Enqueue (pNetBuffer);	// loop back to own address

		return TRUE;
	}

	TEthernetHeader *pHeader =
		(TEthernetHeader *) pNetBuffer->AddHeader (sizeof (TEthernetHeader));

	assert (m_pNetDevLayer != 0);
	const CMACAddress *pOwnMACAddress = m_pNetDevLayer->GetMACAddress ();
	assert (pOwnMACAddress != 0);
	pOwnMACAddress->CopyTo (pHeader->MACSender);

	pHeader->nProtocolType = BE (ETH_PROT_IP);

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
	else if (!m_pARPHandler->Resolve (rReceiver, &MACAddressReceiver, pNetBuffer))
	{
		return TRUE;		// packet will be retransmitted by ARP handler
	}

	MACAddressReceiver.CopyTo (pHeader->MACReceiver);

	m_pNetDevLayer->Send (pNetBuffer);

	return TRUE;
}

CNetBuffer *CLinkLayer::Receive (void)
{
	return m_IPRxQueue.Dequeue ();
}

boolean CLinkLayer::SendRaw (const void *pFrame, unsigned nLength)
{
	assert (pFrame != 0);
	assert (nLength > 0);
	CNetBuffer *pNetBuffer = new CNetBuffer (CNetBuffer::LLRawSend, nLength, pFrame);
	assert (pNetBuffer != 0);

	assert (m_pNetDevLayer != 0);
	m_pNetDevLayer->Send (pNetBuffer);

	return TRUE;
}

boolean CLinkLayer::ReceiveRaw (void *pBuffer, unsigned *pResultLength, CMACAddress *pSender)
{
	CNetBuffer *pNetBuffer = m_RawRxQueue.Dequeue ();
	if (pNetBuffer == 0)
	{
		return FALSE;
	}

	assert (pResultLength != 0);
	*pResultLength = pNetBuffer->GetLength ();

	if (pSender != 0)
	{
		pSender->Set ((const u8 *) pNetBuffer->GetPrivateData ());
	}

	assert (pBuffer != 0);
	memcpy (pBuffer, pNetBuffer->GetPtr (), pNetBuffer->GetLength ());

	delete pNetBuffer;

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

void CLinkLayer::ResolveFailed (CNetBuffer *pReturnedFrame)
{
	assert (pReturnedFrame != 0);
	pReturnedFrame->RemoveHeader (sizeof (TEthernetHeader));

	assert (m_pNetworkLayer != 0);
	m_pNetworkLayer->SendFailed (ICMP_CODE_DEST_HOST_UNREACH, pReturnedFrame);
}
