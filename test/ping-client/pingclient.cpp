//
// pingclient.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2024  R. Stange <rsta2@o2online.de>
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
#include "pingclient.h"
#include <circle/net/checksumcalculator.h>
#include <circle/net/in.h>
#include <circle/macros.h>
#include <circle/util.h>
#include <assert.h>

struct TICMPEchoPacket
{
	u8	nType;
#define ICMP_TYPE_ECHO_REPLY	0
#define ICMP_TYPE_ECHO		8
	u8	nCode;
#define ICMP_CODE_ECHO		0
	u16	nChecksum;
	u16	Identifier;
	u16	SequenceNumber;
	u8	Data[0];
}
PACKED;

unsigned CPingClient::s_nInstanceCount = 0;

CPingClient::CPingClient (CNetSubSystem *pNet)
:	m_pNetworkLayer (pNet->GetNetworkLayer ())
{
	if (!s_nInstanceCount++)
	{
		assert (m_pNetworkLayer);
		m_pNetworkLayer->EnableReceiveICMP (TRUE);
	}
}

CPingClient::~CPingClient (void)
{
	if (!--s_nInstanceCount)
	{
		assert (m_pNetworkLayer);
		m_pNetworkLayer->EnableReceiveICMP (FALSE);
	}
}

boolean CPingClient::Send (const CIPAddress &rServer,
			   u16 usIdentifier, u16 usSequenceNumber,
			   size_t ulLength)
{
	assert (ulLength > sizeof (TICMPEchoPacket));
	u8 Buffer[ulLength];
	TICMPEchoPacket *pPacket = reinterpret_cast<TICMPEchoPacket *> (Buffer);

	pPacket->nType = ICMP_TYPE_ECHO;
	pPacket->nCode = ICMP_CODE_ECHO;
	pPacket->Identifier = le2be16 (usIdentifier);
	pPacket->SequenceNumber = le2be16 (usSequenceNumber);

	for (size_t i = 0; i < ulLength - sizeof (TICMPEchoPacket); i++)
	{
		pPacket->Data[i] = static_cast<u8> (i & 0xFF);
	}

	pPacket->nChecksum = 0;
	pPacket->nChecksum = CChecksumCalculator::SimpleCalculate (Buffer, ulLength);

	assert (m_pNetworkLayer);
	return m_pNetworkLayer->Send (rServer, Buffer, ulLength, IPPROTO_ICMP);
}

boolean CPingClient::Receive (CIPAddress *pSender,
			      u16 *pIdentifier, u16 *pSequenceNumber,
			      size_t *pLength)
{
	u8 Buffer[FRAME_BUFFER_SIZE];
	unsigned nLength;
	CIPAddress Receiver;

	assert (pSender);
	assert (m_pNetworkLayer);
	if (!m_pNetworkLayer->ReceiveICMP (Buffer, &nLength, pSender, &Receiver))
	{
		return FALSE;
	}

	TICMPEchoPacket *pPacket = reinterpret_cast<TICMPEchoPacket *> (Buffer);

	if (   nLength <= sizeof (TICMPEchoPacket)
	    || pPacket->nType != ICMP_TYPE_ECHO_REPLY
	    || pPacket->nCode != ICMP_CODE_ECHO
	    || CChecksumCalculator::SimpleCalculate (Buffer, nLength) != CHECKSUM_OK)
	{
		return FALSE;
	}

	assert (pIdentifier);
	*pIdentifier = be2le16 (pPacket->Identifier);

	assert (pSequenceNumber);
	*pSequenceNumber = be2le16 (pPacket->SequenceNumber);

	assert (pLength);
	*pLength = nLength;

	return TRUE;
}
