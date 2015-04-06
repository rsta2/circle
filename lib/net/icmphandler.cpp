//
// icmphandler.cpp
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
#include <circle/net/icmphandler.h>
#include <circle/net/networklayer.h>
#include <circle/net/checksumcalculator.h>
#include <circle/net/in.h>
#include <circle/usb/netdevice.h>
#include <circle/macros.h>
#include <assert.h>

struct TICMPEchoHeader
{
	u8	nType;
#define ICMP_TYPE_ECHO		8
#define ICMP_TYPE_ECHO_REPLY	0
	u8	nCode;
#define ICMP_CODE_ECHO		0
	u16	nChecksum;
	u16	nIdentifier;
	u16	nSequenceNumber;
}
PACKED;

CICMPHandler::CICMPHandler (CNetConfig *pNetConfig, CNetworkLayer *pNetworkLayer, CNetQueue *pRxQueue)
:	m_pNetConfig (pNetConfig),
	m_pNetworkLayer (pNetworkLayer),
	m_pRxQueue (pRxQueue),
	m_pBuffer (0)
{
	assert (m_pNetConfig != 0);
	assert (m_pNetworkLayer != 0);
	assert (m_pRxQueue != 0);

	m_pBuffer = new u8[FRAME_BUFFER_SIZE];
	assert (m_pBuffer != 0);
}

CICMPHandler::~CICMPHandler (void)
{
	delete [] m_pBuffer;
	m_pBuffer =  0;

	m_pRxQueue = 0;
	m_pNetworkLayer = 0;
	m_pNetConfig = 0;
}

void CICMPHandler::Process (void)
{
	unsigned nLength;
	void *pParam;
	assert (m_pRxQueue != 0);
	assert (m_pBuffer != 0);
	while ((nLength = m_pRxQueue->Dequeue (m_pBuffer, &pParam)) != 0)
	{
		TNetworkPrivateData *pData = (TNetworkPrivateData *) pParam;
		assert (pData != 0);
		assert (pData->nProtocol == IPPROTO_ICMP);

		CIPAddress SourceIP (pData->SourceAddress);
		
		delete pData;
		pData = 0;

		if (nLength < sizeof (TICMPEchoHeader))
		{
			continue;
		}
		TICMPEchoHeader *pHeader = (TICMPEchoHeader *) m_pBuffer;

		if (   pHeader->nType == ICMP_TYPE_ECHO
		    && pHeader->nCode == ICMP_CODE_ECHO
		    && CChecksumCalculator::SimpleCalculate (m_pBuffer, nLength) == CHECKSUM_OK)
		{
			// packet will be used in place to send it back
			pHeader->nType     = ICMP_TYPE_ECHO_REPLY;
			pHeader->nCode     = ICMP_CODE_ECHO;
			pHeader->nChecksum = 0;
			pHeader->nChecksum = CChecksumCalculator::SimpleCalculate (m_pBuffer, nLength);

			assert (m_pNetworkLayer != 0);
			m_pNetworkLayer->Send (SourceIP, m_pBuffer, nLength, IPPROTO_ICMP);
		}
	}
}
