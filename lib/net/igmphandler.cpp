//
// igmphandler.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2025  R. Stange <rsta2@gmx.net>
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
#include <circle/net/igmphandler.h>
#include <circle/net/networklayer.h>
#include <circle/net/checksumcalculator.h>
#include <circle/net/in.h>
#include <circle/util.h>
#include <circle/macros.h>
#include <assert.h>

struct TIGMPPacket
{
	u8	uchType;
#define IGMP_TYPE_MEMBERSHIP_QUERY		0x11
#define IGMP_TYPE_MEMBERSHIP_REPORT_V1		0x12
#define IGMP_TYPE_MEMBERSHIP_REPORT_V2		0x16
#define IGMP_TYPE_LEAVE_GROUP			0x17
	u8	uchMaxRespTime;				// 1/10 seconds, always 0 for v1
	u16	usChecksum;
	u8	GroupAddress[IP_ADDRESS_SIZE];
}
PACKED;

// Multicast addresses
static const u8 AllSystemsGroup[] = {224, 0, 0, 1};
static const u8 AllRoutersGroup[] = {224, 0, 0, 2};

CIGMPHandler::CIGMPHandler (CNetConfig *pNetConfig, CNetworkLayer *pNetworkLayer,
			    CLinkLayer *pLinkLayer, CNetQueue *pRxQueue)
:	m_pNetConfig (pNetConfig),
	m_pNetworkLayer (pNetworkLayer),
	m_pLinkLayer (pLinkLayer),
	m_pRxQueue (pRxQueue),
	m_bAllSystemsJoined (FALSE),
	m_nLastTicks (0),
	m_nV1RouterTimer (0),
	m_nRandomSeed (0)
{
	assert (m_pNetConfig != 0);
	assert (m_pNetworkLayer != 0);
	assert (m_pLinkLayer != 0);
	assert (m_pRxQueue != 0);

	for (unsigned i = 0; i < MaxHostGroups; i++)
	{
		m_nUseCounter[i] = 0;
		m_nDelayTimer[i] = 0;
	}
}

CIGMPHandler::~CIGMPHandler (void)
{
	m_pRxQueue = 0;
	m_pLinkLayer = 0;
	m_pNetworkLayer = 0;
	m_pNetConfig = 0;
}

void CIGMPHandler::Process (void)
{
	assert (m_pLinkLayer != 0);
	if (   !m_bAllSystemsJoined
	    && m_pLinkLayer->IsRunning ())
	{
		CIPAddress AllSystemsGroupIP (AllSystemsGroup);
#ifndef NDEBUG
		boolean bOK =
#endif
			m_pLinkLayer->JoinLocalGroup (AllSystemsGroupIP);
		assert (bOK);

		m_bAllSystemsJoined = TRUE;
	}

	unsigned nTicks = CTimer::Get ()->GetTicks ();

	u8 Buffer[FRAME_BUFFER_SIZE];
	unsigned nLength;
	void *pParam;
	assert (m_pRxQueue != 0);
	while ((nLength = m_pRxQueue->Dequeue (Buffer, &pParam)) != 0)
	{
		TNetworkPrivateData *pData = (TNetworkPrivateData *) pParam;
		assert (pData != 0);
		assert (pData->nProtocol == IPPROTO_IGMP);

		// CIPAddress SourceIP (pData->SourceAddress);
		CIPAddress DestIP (pData->DestinationAddress);

		delete pData;
		pData = 0;

		if (!DestIP.IsMulticast ())
		{
			continue;
		}

		if (nLength < sizeof (TIGMPPacket))
		{
			continue;
		}
		TIGMPPacket *pIGMPPacket = (TIGMPPacket *) Buffer;

		if (CChecksumCalculator::SimpleCalculate (Buffer, nLength) != CHECKSUM_OK)
		{
			continue;
		}

		CIPAddress GroupAddressIP (pIGMPPacket->GroupAddress);

		u8 uchMaxRespTime = pIGMPPacket->uchMaxRespTime;
		if (uchMaxRespTime == 0)
		{
			uchMaxRespTime = 100;

			m_nV1RouterTimer = nTicks + V1RouterPresentTimeout;
		}

		switch (pIGMPPacket->uchType)
		{
		case IGMP_TYPE_MEMBERSHIP_QUERY:
			if (GroupAddressIP.IsNull ())
			{
				// General Query
				for (unsigned i = 0; i < MaxHostGroups; i++)
				{
					if (m_nUseCounter[i] == 0)
					{
						continue;
					}

					m_nDelayTimer[i] =
						nTicks + GetRandom (uchMaxRespTime) * HZ/10;
				}
			}
			else
			{
				// Group-Specific Query
				for (unsigned i = 0; i < MaxHostGroups; i++)
				{
					if (   m_nUseCounter[i] == 0
					    || m_HostGroup[i] != GroupAddressIP)
					{
						continue;
					}

					unsigned nDelayTimer =
						nTicks + GetRandom (uchMaxRespTime) * HZ/10;
					if (   m_nDelayTimer[i] == 0
					    || nDelayTimer < m_nDelayTimer[i])
					{
						m_nDelayTimer[i] = nDelayTimer;
					}

					break;
				}
			}
			break;

		case IGMP_TYPE_MEMBERSHIP_REPORT_V1:
		case IGMP_TYPE_MEMBERSHIP_REPORT_V2:
			for (unsigned i = 0; i < MaxHostGroups; i++)
			{
				if (   m_nUseCounter[i] == 0
				    || m_HostGroup[i] != GroupAddressIP)
				{
					continue;
				}

				if (m_nDelayTimer[i] != 0)
				{
					m_nDelayTimer[i] = 0;
					m_bLastHostFlag[i] = FALSE;
				}

				break;
			}
			break;
		}
	}

	if (nTicks - m_nLastTicks >= HZ/10)
	{
		m_nLastTicks = nTicks;

		// Check for expired timers
		for (unsigned i = 0; i < MaxHostGroups; i++)
		{
			if (   m_nDelayTimer[i] != 0
			    && m_nDelayTimer[i] <= nTicks)
			{
				m_nDelayTimer[i] = 0;
				m_bLastHostFlag[i] = TRUE;

				SendPacket (TRUE, m_HostGroup[i]);
			}
		}

		if (   m_nV1RouterTimer != 0
		    && m_nV1RouterTimer <= nTicks)
		{
			m_nV1RouterTimer = 0;
		}
	}
}

boolean CIGMPHandler::JoinHostGroup (const CIPAddress &rGroupAddress)
{
	unsigned j = MaxHostGroups;
	for (unsigned i = 0; i < MaxHostGroups; i++)
	{
		if (m_nUseCounter[i] == 0)
		{
			if (j == MaxHostGroups)
			{
				j = i;
			}

			continue;
		}

		if (m_HostGroup[i] == rGroupAddress)
		{
			m_nUseCounter[i]++;

			return TRUE;
		}
	}

	if (j == MaxHostGroups)
	{
		return FALSE;
	}

	assert (m_pLinkLayer != 0);
	if (!m_pLinkLayer->JoinLocalGroup (rGroupAddress))
	{
		return FALSE;
	}

	m_HostGroup[j].Set (rGroupAddress);
	m_nUseCounter[j]++;
	m_bLastHostFlag[j] = TRUE;

	// start timer to send Membership Report twice on joining host group
	m_nDelayTimer[j] = CTimer::Get ()->GetTicks () + UnsolicitedReportInterval;

	SendPacket (TRUE, rGroupAddress);

	return TRUE;
}

boolean CIGMPHandler::LeaveHostGroup (const CIPAddress &rGroupAddress)
{
	unsigned i;
	for (i = 0; i < MaxHostGroups; i++)
	{
		if (   m_nUseCounter[i] > 0
		    && m_HostGroup[i] == rGroupAddress)
		{
			if (--m_nUseCounter[i] > 0)
			{
				return TRUE;
			}

			break;
		}
	}

	if (i == MaxHostGroups)
	{
		return FALSE;
	}

	m_nDelayTimer[i] = 0;

	if (m_bLastHostFlag[i])
	{
		SendPacket (FALSE, rGroupAddress);
	}

	assert (m_pLinkLayer != 0);
	m_pLinkLayer->LeaveLocalGroup (rGroupAddress);

	return TRUE;
}

boolean CIGMPHandler::SendPacket (boolean bMembershipReport, const CIPAddress &rGroupAddress)
{
	u8 uchType;
	CIPAddress DestIP;

	if (bMembershipReport)
	{
		uchType =   m_nV1RouterTimer != 0
			  ? IGMP_TYPE_MEMBERSHIP_REPORT_V1
			  : IGMP_TYPE_MEMBERSHIP_REPORT_V2;

		DestIP.Set (rGroupAddress);
	}
	else
	{
		if (m_nV1RouterTimer != 0)
		{
			return TRUE;
		}

		uchType = IGMP_TYPE_LEAVE_GROUP;

		DestIP.Set (AllRoutersGroup);
	}

	TIGMPPacket Packet;
	Packet.uchType = uchType;
	Packet.uchMaxRespTime = 0;

	rGroupAddress.CopyTo (Packet.GroupAddress);

	Packet.usChecksum = 0;
	Packet.usChecksum = CChecksumCalculator::SimpleCalculate (&Packet, sizeof Packet);

	assert (m_pNetworkLayer != 0);
	return m_pNetworkLayer->Send (DestIP, &Packet, sizeof Packet, IPPROTO_IGMP, TRUE);
}

unsigned CIGMPHandler::GetRandom (unsigned nMax)
{
#define RAND_MAX	32767
	m_nRandomSeed = m_nRandomSeed * 1103515245 + 12345;

	unsigned nResult = (m_nRandomSeed / 65536) % 32768;

	return nResult * nMax / RAND_MAX;
}
