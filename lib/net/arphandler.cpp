//
// arphandler.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2020  R. Stange <rsta2@o2online.de>
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
#include <circle/util.h>
#include <circle/macros.h>
#include <assert.h>

// TCP retransmits after 3 seconds, so we finished before
#define ARP_TIMEOUT_HZ		MSEC2HZ (800)
#define ARP_MAX_ATTEMPTS	3

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

CARPHandler::CARPHandler (CNetConfig *pNetConfig, CNetDeviceLayer *pNetDevLayer,
			  CLinkLayer *pLinkLayer, CNetQueue *pRxQueue)
:	m_pNetConfig (pNetConfig),
	m_pNetDevLayer (pNetDevLayer),
	m_pLinkLayer (pLinkLayer),
	m_pRxQueue (pRxQueue),
	m_nEntries (0),
	m_nTicksLastCleanup (0)
{
	assert (m_pNetConfig != 0);
	assert (m_pNetDevLayer != 0);
	assert (m_pLinkLayer != 0);
	assert (m_pRxQueue != 0);
}

CARPHandler::~CARPHandler (void)
{
	for (unsigned nEntry = 0; nEntry < m_nEntries; nEntry++)
	{
		delete m_Entry[nEntry].pTxQueue;
		m_Entry[nEntry].pTxQueue = 0;
	}

	m_pRxQueue = 0;
	m_pNetDevLayer = 0;
	m_pNetConfig = 0;
}

void CARPHandler::Process (void)
{
	assert (m_pNetConfig != 0);
	const CIPAddress *pOwnIPAddress = m_pNetConfig->GetIPAddress ();
	assert (pOwnIPAddress != 0);

	u8 Buffer[FRAME_BUFFER_SIZE];
	TARPPacket *pPacket = (TARPPacket *) Buffer;

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

	assert (m_pLinkLayer != 0);
	assert (m_pNetDevLayer != 0);
	for (unsigned nEntry = 0; nEntry < m_nEntries; nEntry++)
	{
		TARPEntry *pEntry = &m_Entry[nEntry];
		switch (pEntry->State)
		{
		case ARPStateRetryRequest:
			if (pEntry->nAttempts++ < ARP_MAX_ATTEMPTS)
			{
				CIPAddress ForeignIP (pEntry->IPAddress);
				CMACAddress BroadcastAddress;
				BroadcastAddress.SetBroadcast ();
				SendPacket (TRUE, ForeignIP, BroadcastAddress);

				pEntry->State = ARPStateRequestSent;

				pEntry->hTimer = CTimer::Get ()->StartKernelTimer (
								ARP_TIMEOUT_HZ, TimerHandler,
								(void *) (uintptr) nEntry, this);
			}
			else
			{
				assert (pEntry->pTxQueue != 0);
				while ((nResultLength = pEntry->pTxQueue->Dequeue (Buffer)) != 0)
				{
					m_pLinkLayer->ResolveFailed (Buffer, nResultLength);
				}

				pEntry->State = ARPStateFreeSlot;
			}
			break;

		case  ARPStateSendTxQueue:
			assert (pEntry->pTxQueue != 0);
			while ((nResultLength = pEntry->pTxQueue->Dequeue (Buffer)) != 0)
			{
				TEthernetHeader *pHeader = (TEthernetHeader *) Buffer;
				memcpy (pHeader->MACReceiver, pEntry->MACAddress,
					MAC_ADDRESS_SIZE);

				m_pNetDevLayer->Send (Buffer, nResultLength);
			}

			pEntry->State = ARPStateValid;
			break;

		default:
			break;
		}
	}

	unsigned nTicks = CTimer::Get ()->GetTicks ();
	if (nTicks - m_nTicksLastCleanup >= 60*HZ)
	{
		m_nTicksLastCleanup = nTicks;

		m_SpinLock.Acquire ();

		for (unsigned nEntry = 0; nEntry < m_nEntries; nEntry++)
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

boolean CARPHandler::Resolve (const CIPAddress &rIPAddress, CMACAddress *pMACAddress,
			      const void *pFrame, unsigned nFrameLength)
{
	unsigned nFreeSlot = ARP_MAX_ENTRIES;

	unsigned nOldestEntry = -1;
	unsigned nMinTicks = -1;

	m_SpinLock.Acquire ();

	unsigned nEntry;
	for (nEntry = 0; nEntry < m_nEntries; nEntry++)
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
		case ARPStateRetryRequest:
		case ARPStateSendTxQueue:
			if (rIPAddress == m_Entry[nEntry].IPAddress)
			{
				assert (m_Entry[nEntry].pTxQueue != 0);
				m_Entry[nEntry].pTxQueue->Enqueue (pFrame, nFrameLength);

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

	if (nFreeSlot == ARP_MAX_ENTRIES)
	{
		if (m_nEntries < ARP_MAX_ENTRIES)
		{
			nFreeSlot = m_nEntries;
			m_Entry[nFreeSlot].State = ARPStateFreeSlot;

			m_Entry[nFreeSlot].pTxQueue = new CNetQueue;
			assert (m_Entry[nFreeSlot].pTxQueue != 0);

			m_nEntries++;
		}
		else
		{
			assert (nOldestEntry < m_nEntries);
			m_Entry[nOldestEntry].State = ARPStateFreeSlot;

			nFreeSlot = nOldestEntry;
		}
	}
	nEntry = nFreeSlot;
	TARPEntry *pEntry = &m_Entry[nEntry];

	pEntry->State = ARPStateRequestSent;
	rIPAddress.CopyTo (pEntry->IPAddress);

	assert (pEntry->pTxQueue != 0);
	pEntry->pTxQueue->Enqueue (pFrame, nFrameLength);

	pEntry->nTicksLastUsed = CTimer::Get ()->GetTicks ();

	pEntry->nAttempts = 1;

	pEntry->hTimer = CTimer::Get ()->StartKernelTimer (ARP_TIMEOUT_HZ, TimerHandler,
							   (void *) (uintptr) nEntry, this);

	m_SpinLock.Release ();

	CMACAddress BroadcastAddress;
	BroadcastAddress.SetBroadcast ();
	SendPacket (TRUE, rIPAddress, BroadcastAddress);
	
	return FALSE;
}

void CARPHandler::ReplyReceived (const CIPAddress &rForeignIP, const CMACAddress &rForeignMAC)
{
	m_SpinLock.Acquire ();

	for (unsigned nEntry = 0; nEntry < m_nEntries; nEntry++)
	{
		if (   m_Entry[nEntry].State != ARPStateRequestSent
		    && m_Entry[nEntry].State != ARPStateRetryRequest)
		{
			continue;
		}
			
		if (rForeignIP == m_Entry[nEntry].IPAddress)
		{
			CTimer::Get ()->CancelKernelTimer (m_Entry[nEntry].hTimer);

			rForeignMAC.CopyTo (m_Entry[nEntry].MACAddress);
			m_Entry[nEntry].State = ARPStateSendTxQueue;

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
	for (nEntry = 0; nEntry < m_nEntries; nEntry++)
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

	if (   nFreeSlot == ARP_MAX_ENTRIES
	    && m_nEntries < ARP_MAX_ENTRIES)
	{
		nFreeSlot = m_nEntries;
		m_Entry[nFreeSlot].State = ARPStateFreeSlot;

		m_Entry[nFreeSlot].pTxQueue = new CNetQueue;
		assert (m_Entry[nFreeSlot].pTxQueue != 0);

		m_nEntries++;
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

void CARPHandler::TimerHandler (TKernelTimerHandle hTimer, void *pParam, void *pContext)
{
	CARPHandler *pThis = (CARPHandler *) pContext;
	assert (pThis != 0);

	unsigned nEntry = (unsigned) (uintptr) pParam;
	assert (nEntry < pThis->m_nEntries);

	pThis->m_SpinLock.Acquire ();

	if (pThis->m_Entry[nEntry].State == ARPStateRequestSent)
	{
		pThis->m_Entry[nEntry].State = ARPStateRetryRequest;
	}

	pThis->m_SpinLock.Release ();
}
