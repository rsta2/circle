//
// linklayer.cpp
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
#include <circle/net/linklayer.h>
#include <circle/util.h>
#include <assert.h>

CLinkLayer::CLinkLayer (CNetConfig *pNetConfig, CNetDeviceLayer *pNetDevLayer)
:	m_pNetConfig (pNetConfig),
	m_pNetDevLayer (pNetDevLayer),
	m_pARPHandler (0)
{
	assert (m_pNetConfig != 0);
	assert (m_pNetDevLayer != 0);
}

CLinkLayer::~CLinkLayer (void)
{
	delete m_pARPHandler;
	m_pARPHandler = 0;

	m_pNetDevLayer = 0;
	m_pNetConfig = 0;
}

boolean CLinkLayer::Initialize (void)
{
	assert (m_pNetConfig != 0);
	m_pARPHandler = new CARPHandler (m_pNetConfig, m_pNetDevLayer, &m_ARPRxQueue);
	assert (m_pARPHandler != 0);

	return TRUE;
}

void CLinkLayer::Process (void)
{
	assert (m_pNetDevLayer != 0);
	const CMACAddress *pOwnMACAddress = m_pNetDevLayer->GetMACAddress ();
	assert (pOwnMACAddress != 0);

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
			continue;
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
		}
	}

	assert (m_pARPHandler != 0);
	m_pARPHandler->Process ();
}

boolean CLinkLayer::Send (const CIPAddress &rReceiver, const void *pIPPacket, unsigned nLength)
{
	assert (m_pARPHandler != 0);
	CMACAddress MACAddressReceiver;

	assert (m_pNetConfig != 0);
	if (   rReceiver.IsBroadcast ()
	    || rReceiver == *m_pNetConfig->GetBroadcastAddress ())
	{
		MACAddressReceiver.SetBroadcast ();
	}
	else if (!m_pARPHandler->Resolve (rReceiver, &MACAddressReceiver))
	{
		return TRUE;		// ignore packet, will be retransmitted by higher layer
	}

	unsigned nFrameLength = sizeof (TEthernetHeader) + nLength;	// may wrap
	if (   nFrameLength <= sizeof (TEthernetHeader)
	    || nFrameLength > FRAME_BUFFER_SIZE)
	{
		return FALSE;
	}

	u8 FrameBuffer[nFrameLength];
	TEthernetHeader *pHeader = (TEthernetHeader *) FrameBuffer;

	MACAddressReceiver.CopyTo (pHeader->MACReceiver);

	assert (m_pNetDevLayer != 0);
	const CMACAddress *pOwnMACAddress = m_pNetDevLayer->GetMACAddress ();
	assert (pOwnMACAddress != 0);
	pOwnMACAddress->CopyTo (pHeader->MACSender);

	pHeader->nProtocolType = BE (ETH_PROT_IP);

	assert (pIPPacket != 0);
	assert (nLength > 0);
	memcpy (FrameBuffer+sizeof (TEthernetHeader), pIPPacket, nLength);

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
