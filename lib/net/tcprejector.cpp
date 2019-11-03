//
// tcprejector.cpp
//
// Generates RESET response on any received TCP segment
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
#include <circle/net/tcprejector.h>
#include <circle/macros.h>
#include <circle/util.h>
#include <circle/logger.h>
#include <circle/net/in.h>
#include <assert.h>

//#define TCP_DEBUG

struct TTCPHeader
{
	u16 	nSourcePort;
	u16 	nDestPort;
	u32	nSequenceNumber;
	u32	nAcknowledgmentNumber;
	u16	nDataOffsetFlags;		// following #define(s) are valid without BE()
#define TCP_DATA_OFFSET(field)	(((field) >> 4) & 0x0F)
#define TCP_DATA_OFFSET_SHIFT	4
//#define TCP_FLAG_NONCE		(1 << 0)
//#define TCP_FLAG_CWR		(1 << 15)
//#define TCP_FLAG_ECN_ECHO	(1 << 14)
#define TCP_FLAG_URGENT		(1 << 13)
#define TCP_FLAG_ACK		(1 << 12)
#define TCP_FLAG_PUSH		(1 << 11)
#define TCP_FLAG_RESET		(1 << 10)
#define TCP_FLAG_SYN		(1 << 9)
#define TCP_FLAG_FIN		(1 << 8)
	u16	nWindow;
	u16	nChecksum;
	u16	nUrgentPointer;
	u32	Options[0];
}
PACKED;

#ifdef TCP_DEBUG
static const char FromTCP[] = "tcp";
#endif

CTCPRejector::CTCPRejector (CNetConfig *pNetConfig, CNetworkLayer *pNetworkLayer)
:	CNetConnection (pNetConfig, pNetworkLayer, 0, IPPROTO_TCP)
{
}

CTCPRejector::~CTCPRejector (void)
{
}

int CTCPRejector::PacketReceived (const void *pPacket, unsigned nLength,
				  CIPAddress &rSenderIP, CIPAddress &rReceiverIP, int nProtocol)
{
	if (nProtocol != IPPROTO_TCP)
	{
		return 0;
	}

	if (nLength < sizeof (TTCPHeader))
	{
		return -1;
	}

	assert (pPacket != 0);
	TTCPHeader *pHeader = (TTCPHeader *) pPacket;

	m_nOwnPort = be2le16 (pHeader->nDestPort);
	if (m_nOwnPort == 0)
	{
		return -1;
	}

	assert (m_pNetConfig != 0);
	if (m_pNetConfig->GetIPAddress ()->IsNull ())
	{
		return 0;
	}

	m_Checksum.SetSourceAddress (*m_pNetConfig->GetIPAddress ());
	m_Checksum.SetDestinationAddress (rSenderIP);

	if (m_Checksum.Calculate (pPacket, nLength) != CHECKSUM_OK)
	{
		return 0;
	}

	u16 nFlags = pHeader->nDataOffsetFlags;
	u32 nDataOffset = TCP_DATA_OFFSET (pHeader->nDataOffsetFlags)*4;
	u32 nDataLength = nLength-nDataOffset;

	// Current Segment Variables
	u32 nSEG_SEQ = be2le32 (pHeader->nSequenceNumber);
	u32 nSEG_ACK = be2le32 (pHeader->nAcknowledgmentNumber);

	u32 nSEG_LEN = nDataLength;
	if (nFlags & TCP_FLAG_SYN)
	{
		nSEG_LEN++;
	}
	if (nFlags & TCP_FLAG_FIN)
	{
		nSEG_LEN++;
	}
	
#ifdef TCP_DEBUG
	u32 nSEG_WND = be2le16 (pHeader->nWindow);

	CLogger::Get ()->Write (FromTCP, LogDebug,
				"rx %c%c%c%c%c%c, seq %u, ack %u, win %u, len %u",
				nFlags & TCP_FLAG_URGENT ? 'U' : '-',
				nFlags & TCP_FLAG_ACK    ? 'A' : '-',
				nFlags & TCP_FLAG_PUSH   ? 'P' : '-',
				nFlags & TCP_FLAG_RESET  ? 'R' : '-',
				nFlags & TCP_FLAG_SYN    ? 'S' : '-',
				nFlags & TCP_FLAG_FIN    ? 'F' : '-',
				nSEG_SEQ,
				nFlags & TCP_FLAG_ACK ? nSEG_ACK : 0,
				nSEG_WND,
				nDataLength);
#endif

	m_ForeignIP.Set (rSenderIP);
	m_nForeignPort = be2le16 (pHeader->nSourcePort);

	if (nFlags & TCP_FLAG_RESET)
	{
		// ignore
	}
	else if (!(nFlags & TCP_FLAG_ACK))
	{
		SendSegment (TCP_FLAG_RESET | TCP_FLAG_ACK, 0, nSEG_SEQ+nSEG_LEN);
	}
	else
	{
		SendSegment (TCP_FLAG_RESET, nSEG_ACK);
	}

	return 1;
}

boolean CTCPRejector::SendSegment (unsigned nFlags, u32 nSequenceNumber, u32 nAcknowledgmentNumber)
{
	assert (!(nFlags & TCP_FLAG_SYN));

	unsigned nDataOffset = 5;
	assert (nDataOffset * 4 == sizeof (TTCPHeader));

	unsigned nHeaderLength = nDataOffset * 4;
	unsigned nPacketLength = nHeaderLength;
	assert (nHeaderLength <= FRAME_BUFFER_SIZE);

	u8 TxBuffer[FRAME_BUFFER_SIZE];
	TTCPHeader *pHeader = (TTCPHeader *) TxBuffer;

	pHeader->nSourcePort	 	= le2be16 (m_nOwnPort);
	pHeader->nDestPort	 	= le2be16 (m_nForeignPort);
	pHeader->nSequenceNumber 	= le2be32 (nSequenceNumber);
	pHeader->nAcknowledgmentNumber	= nFlags & TCP_FLAG_ACK ? le2be32 (nAcknowledgmentNumber) : 0;
	pHeader->nDataOffsetFlags	= (nDataOffset << TCP_DATA_OFFSET_SHIFT) | nFlags;
	pHeader->nWindow		= 0;
	pHeader->nUrgentPointer		= 0;

	pHeader->nChecksum = 0;		// must be 0 for calculation
	pHeader->nChecksum = m_Checksum.Calculate (TxBuffer, nPacketLength);

#ifdef TCP_DEBUG
	CLogger::Get ()->Write (FromTCP, LogDebug,
				"tx %c%c%c%c%c%c, seq %u, ack %u, win %u, len %u",
				nFlags & TCP_FLAG_URGENT ? 'U' : '-',
				nFlags & TCP_FLAG_ACK    ? 'A' : '-',
				nFlags & TCP_FLAG_PUSH   ? 'P' : '-',
				nFlags & TCP_FLAG_RESET  ? 'R' : '-',
				nFlags & TCP_FLAG_SYN    ? 'S' : '-',
				nFlags & TCP_FLAG_FIN    ? 'F' : '-',
				nSequenceNumber,
				nFlags & TCP_FLAG_ACK ? nAcknowledgmentNumber : 0,
				0,
				0);
#endif

	assert (m_pNetworkLayer != 0);
	return m_pNetworkLayer->Send (m_ForeignIP, TxBuffer, nPacketLength, IPPROTO_TCP);
}
