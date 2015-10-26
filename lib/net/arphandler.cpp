//
// arphandler.cpp
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
#include <circle/net/arphandler.h>
#include <circle/net/linklayer.h>
#include <circle/timer.h>
#include <circle/util.h>
#include <circle/macros.h>
#include <assert.h>

#define ARP_TIMEOUT_HZ		MSEC2HZ (500)

#define ARP_LIFETIME_HZ		(600 * HZ)

struct TARPPacket
{
	u16		nHWAddressSpace;
#define HW_ADDR_ETHER		1
	u16		nProtocolAddressSpace;
#define PROT_ADDR_IP		0x800
	u8		nHWAddressLength;
	u8		nProtocolAddressLength;
	u16		nOPCode;
#define ARP_REQUEST		1
#define ARP_REPLY		2
	u8		HWAddressSender[MAC_ADDRESS_SIZE];
	u8		ProtocolAddressSender[IP_ADDRESS_SIZE];
	u8		HWAddressTarget[MAC_ADDRESS_SIZE];
	u8		ProtocolAddressTarget[IP_ADDRESS_SIZE];
}
PACKED;

struct TARPFrame
{
	TEthernetHeader Ethernet;
	TARPPacket	ARP;
}
PACKED;

CARPHandler::CARPHandler (CNetConfig *pNetConfig, CNetDeviceLayer *pNetDevLayer, CNetQueue *pRxQueue)
:	m_pNetConfig (pNetConfig),
	m_pNetDevLayer (pNetDevLayer),
	m_pRxQueue (pRxQueue),
	m_pBuffer (0)
{
	assert (m_pNetConfig != 0);
	assert (m_pNetDevLayer != 0);
	assert (m_pRxQueue != 0);

	for (unsigned nEntry = 0; nEntry < ARP_MAX_ENTRIES; nEntry++)
	{
		m_Entry[nEntry].State = ARPStateFreeSlot;
	}

	m_pBuffer = new u8[FRAME_BUFFER_SIZE];
	assert (m_pBuffer != 0);
}

CARPHandler::~CARPHandler (void)
{
	delete [] m_pBuffer;
	m_pBuffer =  0;

	m_pRxQueue = 0;
	m_pNetDevLayer = 0;
	m_pNetConfig = 0;
}

void CARPHandler::Process (void)
{
	assert (m_pNetConfig != 0);
	const CIPAddress *pOwnIPAddress = m_pNetConfig->GetIPAddress ();
	assert (pOwnIPAddress != 0);

	TARPPacket *pPacket = (TARPPacket *) m_pBuffer;
	assert (pPacket != 0);
	
	u32 nResultLength;
	assert (m_pRxQueue != 0);
	while ((nResultLength = m_pRxQueue->Dequeue (pPacket)) != 0)
	{
		if (nResultLength <  sizeof (TARPPacket))
		{
			continue;
		}

		if (   pPacket->nHWAddressSpace        != BE (HW_ADDR_ETHER)
		    || pPacket->nProtocolAddressSpace  != BE (PROT_ADDR_IP)
		    || pPacket->nHWAddressLength       != MAC_ADDRESS_SIZE
		    || pPacket->nProtocolAddressLength != IP_ADDRESS_SIZE)
		{
			continue;
		}

		if (   pOwnIPAddress->IsNull ()
		    || *pOwnIPAddress != pPacket->ProtocolAddressTarget)
		{
			continue;
		}

		CMACAddress MACAddressSender (pPacket->HWAddressSender);
		CIPAddress IPAddressSender (pPacket->ProtocolAddressSender);
		
		switch (pPacket->nOPCode)
		{
		case BE (ARP_REQUEST):
			SendPacket (FALSE, IPAddressSender, MACAddressSender);
			RequestReceived (IPAddressSender, MACAddressSender);
			break;

		case BE (ARP_REPLY):
			ReplyReceived (IPAddressSender, MACAddressSender);
			break;

		default:
			continue;
		}
	}

	unsigned long nTicks = CTimer::Get ()->GetTicks ();
	if ((nTicks / HZ) % 60 == 0)
	{
		m_SpinLock.Acquire ();

		for (unsigned nEntry = 0; nEntry < ARP_MAX_ENTRIES; nEntry++)
		{
			if (   m_Entry[nEntry].State == ARPStateValid
			    && m_Entry[nEntry].nTicksLastUsed + ARP_LIFETIME_HZ < nTicks)
			{
				m_Entry[nEntry].State = ARPStateFreeSlot;
			}
		}

		m_SpinLock.Release ();
	}
}

boolean CARPHandler::Resolve (const CIPAddress &rIPAddress, CMACAddress *pMACAddress)
{
	unsigned nFreeSlot = ARP_MAX_ENTRIES;

	unsigned nOldestEntry = -1;
	unsigned nMinTicks = -1;

	m_SpinLock.Acquire ();

	unsigned nEntry;
	for (nEntry = 0; nEntry < ARP_MAX_ENTRIES; nEntry++)
	{
		switch (m_Entry[nEntry].State)
		{
		case ARPStateFreeSlot:
			if (nFreeSlot == ARP_MAX_ENTRIES)
			{
				nFreeSlot = nEntry;
			}
			break;

		case ARPStateRequestSent:
			if (rIPAddress == m_Entry[nEntry].IPAddress)
			{
				m_Entry[nEntry].nTicksLastUsed = CTimer::Get ()->GetTicks ();

				m_SpinLock.Release ();

				return FALSE;
			}
			break;

		case ARPStateValid:
			if (m_Entry[nEntry].nTicksLastUsed < nMinTicks)
			{
				nOldestEntry = nEntry;
				nMinTicks = m_Entry[nEntry].nTicksLastUsed;
			}

			if (rIPAddress == m_Entry[nEntry].IPAddress)
			{
				assert (pMACAddress != 0);
				pMACAddress->Set (m_Entry[nEntry].MACAddress);
				m_Entry[nEntry].nTicksLastUsed = CTimer::Get ()->GetTicks ();

				m_SpinLock.Release ();

				return TRUE;
			}
			break;

		default:
			assert (0);
			break;
		}
	}

	assert (nEntry == ARP_MAX_ENTRIES);
	if (nFreeSlot == ARP_MAX_ENTRIES)
	{
		assert (nOldestEntry < ARP_MAX_ENTRIES);
		m_Entry[nOldestEntry].State = ARPStateFreeSlot;
		nFreeSlot = nOldestEntry;
	}
	nEntry = nFreeSlot;
	TARPEntry *pEntry = &m_Entry[nEntry];

	pEntry->State = ARPStateRequestSent;
	rIPAddress.CopyTo (pEntry->IPAddress);

	pEntry->nTicksLastUsed = CTimer::Get ()->GetTicks ();

	pEntry->hTimer = CTimer::Get ()->StartKernelTimer (ARP_TIMEOUT_HZ, TimerHandler, (void *) nEntry, this);

	m_SpinLock.Release ();

	CMACAddress BroadcastAddress;
	BroadcastAddress.SetBroadcast ();
	SendPacket (TRUE, rIPAddress, BroadcastAddress);
	
	return FALSE;
}

void CARPHandler::ReplyReceived (const CIPAddress &rForeignIP, const CMACAddress &rForeignMAC)
{
	m_SpinLock.Acquire ();

	for (unsigned nEntry = 0; nEntry < ARP_MAX_ENTRIES; nEntry++)
	{
		if (m_Entry[nEntry].State != ARPStateRequestSent)
		{
			continue;
		}
			
		if (rForeignIP == m_Entry[nEntry].IPAddress)
		{
			CTimer::Get ()->CancelKernelTimer (m_Entry[nEntry].hTimer);

			rForeignMAC.CopyTo (m_Entry[nEntry].MACAddress);
			m_Entry[nEntry].State = ARPStateValid;

			break;
		}
	}

	m_SpinLock.Release ();
}

void CARPHandler::RequestReceived (const CIPAddress &rForeignIP, const CMACAddress &rForeignMAC)
{
	m_SpinLock.Acquire ();

	unsigned nFreeSlot = ARP_MAX_ENTRIES;
	unsigned nEntry;
	for (nEntry = 0; nEntry < ARP_MAX_ENTRIES; nEntry++)
	{
		if (m_Entry[nEntry].State == ARPStateFreeSlot)
		{
			if (nFreeSlot == ARP_MAX_ENTRIES)
			{
				nFreeSlot = nEntry;
			}
		}
		else if (rForeignIP == m_Entry[nEntry].IPAddress)
		{
			m_SpinLock.Release ();

			return;
		}
	}

	if (nFreeSlot < ARP_MAX_ENTRIES)
	{
		rForeignIP.CopyTo (m_Entry[nFreeSlot].IPAddress);
		rForeignMAC.CopyTo (m_Entry[nFreeSlot].MACAddress);

		m_Entry[nFreeSlot].nTicksLastUsed = CTimer::Get ()->GetTicks ();

		m_Entry[nFreeSlot].State = ARPStateValid;
	}

	m_SpinLock.Release ();
}

void CARPHandler::SendPacket (boolean		 bRequest,
			      const CIPAddress	&rForeignIP,
			      const CMACAddress	&rForeignMAC)
{
	assert (m_pNetConfig != 0);
	const CIPAddress *pOwnIPAddress = m_pNetConfig->GetIPAddress ();
	assert (pOwnIPAddress != 0);
	
	assert (m_pNetDevLayer != 0);
	const CMACAddress *pOwnMACAddress = m_pNetDevLayer->GetMACAddress ();
	assert (pOwnMACAddress != 0);

	TARPFrame ARPFrame;

	rForeignMAC.CopyTo (ARPFrame.Ethernet.MACReceiver);
	pOwnMACAddress->CopyTo (ARPFrame.Ethernet.MACSender);
	ARPFrame.Ethernet.nProtocolType = BE (ETH_PROT_ARP);
	
	ARPFrame.ARP.nHWAddressSpace        = BE (HW_ADDR_ETHER);
	ARPFrame.ARP.nProtocolAddressSpace  = BE (PROT_ADDR_IP);
	ARPFrame.ARP.nHWAddressLength       = MAC_ADDRESS_SIZE;
	ARPFrame.ARP.nProtocolAddressLength = IP_ADDRESS_SIZE;
	ARPFrame.ARP.nOPCode                = bRequest ? BE (ARP_REQUEST) : BE (ARP_REPLY);

	pOwnMACAddress->CopyTo (ARPFrame.ARP.HWAddressSender);
	pOwnIPAddress->CopyTo (ARPFrame.ARP.ProtocolAddressSender);
	rForeignMAC.CopyTo (ARPFrame.ARP.HWAddressTarget);
	rForeignIP.CopyTo (ARPFrame.ARP.ProtocolAddressTarget);

	m_pNetDevLayer->Send (&ARPFrame, sizeof ARPFrame);
}

void CARPHandler::TimerHandler (unsigned hTimer, void *pParam, void *pContext)
{
	CARPHandler *pThis = (CARPHandler *) pContext;
	assert (pThis != 0);

	unsigned nEntry = (unsigned) pParam;
	assert (nEntry < ARP_MAX_ENTRIES);

	pThis->m_SpinLock.Acquire ();

	if (pThis->m_Entry[nEntry].State == ARPStateRequestSent)
	{
		pThis->m_Entry[nEntry].State = ARPStateFreeSlot;
	}

	pThis->m_SpinLock.Release ();
}
