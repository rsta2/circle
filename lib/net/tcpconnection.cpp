//
// tcpconnection.cpp
//
// This implements RFC 793 with some changes in RFC 1122 and RFC 6298.
//
// Non-implemented features:
//	dynamic receive window
//	URG flag and urgent pointer
//	delayed ACK
//	queueing out-of-order TCP segments
//	security/compartment
//	precedence
//	user timeout
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/net/tcpconnection.h>
#include <circle/macros.h>
#include <circle/util.h>
#include <circle/logger.h>
#include <circle/net/in.h>
#include <assert.h>

//#define TCP_DEBUG

#define TCP_MAX_CONNECTIONS		1000	// maximum number of active TCP connections

#define MSS_R				1480	// maximum segment size to be received from network layer
#define MSS_S				1480	// maximum segment size to be send to network layer

#define TCP_CONFIG_MSS			(MSS_R - 20)
#define TCP_CONFIG_WINDOW		(TCP_CONFIG_MSS * 10)

#define TCP_CONFIG_RETRANS_BUFFER_SIZE	0x10000	// should be greater than maximum send window size

#define TCP_MAX_WINDOW			((u16) -1)	// without Window extension option
#define TCP_QUIET_TIME			30	// seconds after crash before another connection starts

#define HZ_TIMEWAIT			(60 * HZ)
#define HZ_FIN_TIMEOUT			(60 * HZ)	// timeout in FIN-WAIT-2 state

#define MAX_RETRANSMISSIONS		5

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
	u32	Options[];
}
PACKED;

#define TCP_HEADER_SIZE		20		// valid for normal data segments without TCP options

struct TTCPOption
{
	u8	nKind;			// Data:
#define TCP_OPTION_END_OF_LIST	0	//	None (no length field)
#define TCP_OPTION_NOP		1	//	None (no length field)
#define TCP_OPTION_MSS		2	//	Maximum segment size (2 byte)
#define TCP_OPTION_WINDOW_SCALE	3	//	Shift count (1 byte)
#define TCP_OPTION_SACK_PERM	4	//	None
#define TCP_OPTION_TIMESTAMP	8	//	Timestamp value, Timestamp echo reply (2*4 byte)
	u8	nLength;
	u8	Data[];
}
PACKED;

#define min(n, m)		((n) <= (m) ? (n) : (m))
#define max(n, m)		((n) >= (m) ? (n) : (m))

// Modulo 32 sequence number arithmetic
#define lt(x, y)		((int) ((u32) (x) - (u32) (y)) < 0)
#define le(x, y)		((int) ((u32) (x) - (u32) (y)) <= 0)
#define gt(x, y) 		lt (y, x)
#define ge(x, y) 		le (y, x)

#define bw(l, x, h)		(lt ((l), (x)) && lt ((x), (h)))	// between
#define bwl(l, x, h)		(le ((l), (x)) && lt ((x), (h)))	//	low border inclusive
#define bwh(l, x, h)		(lt ((l), (x)) && le ((x), (h)))	//	high border inclusive
#define bwlh(l, x, h)		(le ((l), (x)) && le ((x), (h)))	//	both borders inclusive

#if !defined (NDEBUG) && defined (TCP_DEBUG)
	#define NEW_STATE(state)	NewState (state, __LINE__);
#else
	#define NEW_STATE(state)	(m_State = state)
#endif

#ifndef NDEBUG
	#define UNEXPECTED_STATE()	UnexpectedState (__LINE__)
#else
	#define UNEXPECTED_STATE()	((void) 0)
#endif

unsigned CTCPConnection::s_nConnections = 0;

static const char FromTCP[] = "tcp";

CTCPConnection::CTCPConnection (CNetConfig	*pNetConfig,
				CNetworkLayer	*pNetworkLayer,
				CIPAddress	&rForeignIP,
				u16		 nForeignPort,
				u16		 nOwnPort)
:	CNetConnection (pNetConfig, pNetworkLayer, rForeignIP, nForeignPort, nOwnPort, IPPROTO_TCP),
	m_bActiveOpen (TRUE),
	m_State (TCPStateClosed),
	m_nErrno (0),
	m_RetransmissionQueue (TCP_CONFIG_RETRANS_BUFFER_SIZE),
	m_bRetransmit (FALSE),
	m_bSendSYN (FALSE),
	m_bFINQueued (FALSE),
	m_nRetransmissionCount (0),
	m_bTimedOut (FALSE),
	m_pTimer (CTimer::Get ()),
	m_nSND_WND (TCP_CONFIG_WINDOW),
	m_nSND_UP (0),
	m_nRCV_NXT (0),
	m_nRCV_WND (TCP_CONFIG_WINDOW),
	m_nIRS (0),
	m_nSND_MSS (536)	// RFC 1122 section 4.2.2.6
{
	s_nConnections++;

	for (unsigned nTimer = TCPTimerUser; nTimer < TCPTimerUnknown; nTimer++)
	{
		m_hTimer[nTimer] = 0;
	}

	m_nISS = CalculateISN ();
	m_RTOCalculator.Initialize (m_nISS);

	m_nSND_UNA = m_nISS;
	m_nSND_NXT = m_nISS+1;

	if (SendSegment (TCP_FLAG_SYN, m_nISS))
	{
		m_RTOCalculator.SegmentSent (m_nISS);
		NEW_STATE (TCPStateSynSent);
		m_nRetransmissionCount = MAX_RETRANSMISSIONS;
		StartTimer (TCPTimerRetransmission, m_RTOCalculator.GetRTO ());
	}
}

CTCPConnection::CTCPConnection (CNetConfig	*pNetConfig,
				CNetworkLayer	*pNetworkLayer,
				u16		 nOwnPort)
:	CNetConnection (pNetConfig, pNetworkLayer, nOwnPort, IPPROTO_TCP),
	m_bActiveOpen (FALSE),
	m_State (TCPStateListen),
	m_nErrno (0),
	m_RetransmissionQueue (TCP_CONFIG_RETRANS_BUFFER_SIZE),
	m_bRetransmit (FALSE),
	m_bSendSYN (FALSE),
	m_bFINQueued (FALSE),
	m_nRetransmissionCount (0),
	m_bTimedOut (FALSE),
	m_pTimer (CTimer::Get ()),
	m_nSND_WND (TCP_CONFIG_WINDOW),
	m_nSND_UP (0),
	m_nRCV_NXT (0),
	m_nRCV_WND (TCP_CONFIG_WINDOW),
	m_nIRS (0),
	m_nSND_MSS (536)	// RFC 1122 section 4.2.2.6
{
	s_nConnections++;

	for (unsigned nTimer = TCPTimerUser; nTimer < TCPTimerUnknown; nTimer++)
	{
		m_hTimer[nTimer] = 0;
	}
}

CTCPConnection::~CTCPConnection (void)
{
#ifdef TCP_DEBUG
	CLogger::Get ()->Write (FromTCP, LogDebug, "Delete TCB");
#endif

	assert (m_State == TCPStateClosed);

	for (unsigned nTimer = TCPTimerUser; nTimer < TCPTimerUnknown; nTimer++)
	{
		StopTimer (nTimer);
	}

	// ensure no task is waiting any more
	m_Event.Set ();
	m_TxEvent.Set ();

	assert (s_nConnections > 0);
	s_nConnections--;
}

int CTCPConnection::Connect (void)
{
	if (m_nErrno < 0)
	{
		return m_nErrno;
	}

	switch (m_State)
	{
	case TCPStateSynSent:
	case TCPStateSynReceived:
		m_Event.Clear ();
		m_Event.Wait ();
		break;

	case TCPStateEstablished:
		break;

	case TCPStateListen:
	case TCPStateFinWait1:
	case TCPStateFinWait2:
	case TCPStateCloseWait:
	case TCPStateClosing:
	case TCPStateLastAck:
	case TCPStateTimeWait:
		UNEXPECTED_STATE ();
		// fall through

	case TCPStateClosed:
		return -1;
	}

	return m_nErrno;
}

int CTCPConnection::Accept (CIPAddress *pForeignIP, u16 *pForeignPort)
{
	if (m_nErrno < 0)
	{
		return m_nErrno;
	}

	switch (m_State)
	{
	case TCPStateSynSent:
		UNEXPECTED_STATE ();
		// fall through

	case TCPStateClosed:
	case TCPStateFinWait1:
	case TCPStateFinWait2:
	case TCPStateCloseWait:
	case TCPStateClosing:
	case TCPStateLastAck:
	case TCPStateTimeWait:
		return -1;

	case TCPStateListen:
		m_Event.Clear ();
		m_Event.Wait ();
		break;

	case TCPStateSynReceived:
	case TCPStateEstablished:
		break;
	}

	assert (pForeignIP != 0);
	pForeignIP->Set (m_ForeignIP);

	assert (pForeignPort != 0);
	*pForeignPort = m_nForeignPort;

	return m_nErrno;
}

int CTCPConnection::Close (void)
{
	if (m_nErrno < 0)
	{
		return m_nErrno;
	}
	
	switch (m_State)
	{
	case TCPStateClosed:
		return -1;

	case TCPStateListen:
	case TCPStateSynSent:
		StopTimer (TCPTimerRetransmission);
		NEW_STATE (TCPStateClosed);
		break;

	case TCPStateSynReceived:
	case TCPStateEstablished:
		assert (!m_bFINQueued);
		m_StateAfterFIN = TCPStateFinWait1;
		m_nRetransmissionCount = MAX_RETRANSMISSIONS;
		m_bFINQueued = TRUE;
		break;
		
	case TCPStateFinWait1:
	case TCPStateFinWait2:
		break;

	case TCPStateCloseWait:
		assert (!m_bFINQueued);
		m_StateAfterFIN = TCPStateLastAck;	// RFC 1122 section 4.2.2.20 (a)
		m_nRetransmissionCount = MAX_RETRANSMISSIONS;
		m_bFINQueued = TRUE;
		break;

	case TCPStateClosing:
	case TCPStateLastAck:
	case TCPStateTimeWait:
		return -1;
	}

	if (m_nErrno < 0)
	{
		return m_nErrno;
	}
	
	return 0;
}

int CTCPConnection::Send (const void *pData, unsigned nLength, int nFlags)
{
	if (   nFlags != 0
	    && nFlags != MSG_DONTWAIT)
	{
		return -1;
	}

	if (m_nErrno < 0)
	{
		return m_nErrno;
	}
	
	switch (m_State)
	{
	case TCPStateClosed:
	case TCPStateListen:
	case TCPStateFinWait1:
	case TCPStateFinWait2:
	case TCPStateClosing:
	case TCPStateLastAck:
	case TCPStateTimeWait:
		return -1;

	case TCPStateSynSent:
	case TCPStateSynReceived:
	case TCPStateEstablished:
	case TCPStateCloseWait:
		break;
	}
	
	unsigned nResult = nLength;

	assert (pData != 0);
	u8 *pBuffer = (u8 *) pData;

	while (nLength > FRAME_BUFFER_SIZE)
	{
		m_TxQueue.Enqueue (pBuffer, FRAME_BUFFER_SIZE);

		pBuffer += FRAME_BUFFER_SIZE;
		nLength -= FRAME_BUFFER_SIZE;
	}

	if (nLength > 0)
	{
		m_TxQueue.Enqueue (pBuffer, nLength);
	}

	if (!(nFlags & MSG_DONTWAIT))
	{
		m_TxEvent.Clear ();
		m_TxEvent.Wait ();

		if (m_nErrno < 0)
		{
			return m_nErrno;
		}
	}
	
	return nResult;
}

int CTCPConnection::Receive (void *pBuffer, int nFlags)
{
	if (   nFlags != 0
	    && nFlags != MSG_DONTWAIT)
	{
		return -1;
	}

	if (m_nErrno < 0)
	{
		return m_nErrno;
	}
	
	unsigned nLength;
	while ((nLength = m_RxQueue.Dequeue (pBuffer)) == 0)
	{
		switch (m_State)
		{
		case TCPStateClosed:
		case TCPStateListen:
		case TCPStateFinWait1:
		case TCPStateFinWait2:
		case TCPStateCloseWait:
		case TCPStateClosing:
		case TCPStateLastAck:
		case TCPStateTimeWait:
			return -1;

		case TCPStateSynSent:
		case TCPStateSynReceived:
		case TCPStateEstablished:
			break;
		}

		if (nFlags & MSG_DONTWAIT)
		{
			return 0;
		}

		m_Event.Clear ();
		m_Event.Wait ();

		if (m_nErrno < 0)
		{
			return m_nErrno;
		}
	}

	return nLength;
}

int CTCPConnection::SendTo (const void *pData, unsigned nLength, int nFlags,
			    CIPAddress	&rForeignIP, u16 nForeignPort)
{
	// ignore rForeignIP and nForeignPort
	return Send (pData, nLength, nFlags);
}

int CTCPConnection::ReceiveFrom (void *pBuffer, int nFlags, CIPAddress *pForeignIP, u16 *pForeignPort)
{
	int nResult = Receive (pBuffer, nFlags);
	if (nResult <= 0)
	{
		return nResult;
	}

	if (   pForeignIP != 0
	    && pForeignPort != 0)
	{
		pForeignIP->Set (m_ForeignIP);
		*pForeignPort = m_nForeignPort;
	}

	return 0;
}

int CTCPConnection::SetOptionBroadcast (boolean bAllowed)
{
	return 0;
}

boolean CTCPConnection::IsConnected (void) const
{
	return     m_State > TCPStateSynSent
		&& m_State != TCPStateTimeWait;
}

boolean CTCPConnection::IsTerminated (void) const
{
	return m_State == TCPStateClosed;
}

void CTCPConnection::Process (void)
{
	if (m_bTimedOut)
	{
		m_nErrno = -1;
		NEW_STATE (TCPStateClosed);
		m_Event.Set ();
		return;
	}

	switch (m_State)
	{
	case TCPStateClosed:
	case TCPStateListen:
	case TCPStateFinWait2:
	case TCPStateTimeWait:
		return;

	case TCPStateSynSent:
	case TCPStateSynReceived:
		if (m_bSendSYN)
		{
			m_bSendSYN = FALSE;
			if (m_State == TCPStateSynSent)
			{
				SendSegment (TCP_FLAG_SYN, m_nISS);
			}
			else
			{
				SendSegment (TCP_FLAG_SYN | TCP_FLAG_ACK, m_nISS, m_nRCV_NXT);
			}
			m_RTOCalculator.SegmentSent (m_nISS);
			StartTimer (TCPTimerRetransmission, m_RTOCalculator.GetRTO ());
		}
		return;
		
	case TCPStateEstablished:
	case TCPStateFinWait1:
	case TCPStateCloseWait:
	case TCPStateClosing:
	case TCPStateLastAck:
		if (   m_RetransmissionQueue.IsEmpty ()
		    && m_TxQueue.IsEmpty ()
		    && m_bFINQueued)
		{
			SendSegment (TCP_FLAG_FIN | TCP_FLAG_ACK, m_nSND_NXT, m_nRCV_NXT);
			m_RTOCalculator.SegmentSent (m_nSND_NXT);
			m_nSND_NXT++;
			NEW_STATE (m_StateAfterFIN);
			m_bFINQueued = FALSE;
			StartTimer (TCPTimerRetransmission, m_RTOCalculator.GetRTO ());
		}
		break;
	}

	u8 TempBuffer[FRAME_BUFFER_SIZE];
	unsigned nLength;
	while (    m_RetransmissionQueue.GetFreeSpace () >= FRAME_BUFFER_SIZE
		&& (nLength = m_TxQueue.Dequeue (TempBuffer)) > 0)
	{
#ifdef TCP_DEBUG
		CLogger::Get ()->Write (FromTCP, LogDebug, "Transfering %u bytes into RT buffer", nLength);
#endif

		m_RetransmissionQueue.Write (TempBuffer, nLength);
	}

	// pacing transmit
	if (   (   m_State == TCPStateEstablished
		|| m_State == TCPStateCloseWait)
	    && m_TxQueue.IsEmpty ())
	{
		m_TxEvent.Set ();
	}

	if (m_bRetransmit)
	{
#ifdef TCP_DEBUG
		CLogger::Get ()->Write (FromTCP, LogDebug, "Retransmission (nxt %u, una %u)", m_nSND_NXT-m_nISS, m_nSND_UNA-m_nISS);
#endif
		m_bRetransmit = FALSE;
		m_RetransmissionQueue.Reset ();
		m_nSND_NXT = m_nSND_UNA;
	}

	u32 nBytesAvail;
	u32 nWindowLeft;
	while (   (nBytesAvail = m_RetransmissionQueue.GetBytesAvailable ()) > 0
	       && (nWindowLeft = m_nSND_UNA+m_nSND_WND-m_nSND_NXT) > 0)
	{
		nLength = min (nBytesAvail, nWindowLeft);
		nLength = min (nLength, m_nSND_MSS);

#ifdef TCP_DEBUG
		CLogger::Get ()->Write (FromTCP, LogDebug, "Transfering %u bytes into TX buffer", nLength);
#endif

		assert (nLength <= FRAME_BUFFER_SIZE);
		m_RetransmissionQueue.Read (TempBuffer, nLength);

		unsigned nFlags = TCP_FLAG_ACK;
		if (m_TxQueue.IsEmpty ())
		{
			nFlags |= TCP_FLAG_PUSH;
		}

		SendSegment (nFlags, m_nSND_NXT, m_nRCV_NXT, TempBuffer, nLength);
		m_RTOCalculator.SegmentSent (m_nSND_NXT, nLength);
		m_nSND_NXT += nLength;
		StartTimer (TCPTimerRetransmission, m_RTOCalculator.GetRTO ());
	}
}

int CTCPConnection::PacketReceived (const void	*pPacket,
				    unsigned	 nLength,
				    CIPAddress	&rSenderIP,
				    CIPAddress	&rReceiverIP,
				    int		 nProtocol)
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

	if (m_nOwnPort != be2le16 (pHeader->nDestPort))
	{
		return 0;
	}
	
	if (m_State != TCPStateListen)
	{
		if (   m_ForeignIP != rSenderIP
		    || m_nForeignPort != be2le16 (pHeader->nSourcePort))
		{
			return 0;
		}
	}
	else
	{
		if (!(pHeader->nDataOffsetFlags & TCP_FLAG_SYN))
		{
			return 0;
		}

		m_Checksum.SetDestinationAddress (rSenderIP);
	}

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
	
	u32 nSEG_WND = be2le16 (pHeader->nWindow);
	//u16 nSEG_UP  = be2le16 (pHeader->nUrgentPointer);
	//u32 nSEG_PRC;	// segment precedence value

	ScanOptions (pHeader);

#ifdef TCP_DEBUG
	CLogger::Get ()->Write (FromTCP, LogDebug,
				"rx %c%c%c%c%c%c, seq %u, ack %u, win %u, len %u",
				nFlags & TCP_FLAG_URGENT ? 'U' : '-',
				nFlags & TCP_FLAG_ACK    ? 'A' : '-',
				nFlags & TCP_FLAG_PUSH   ? 'P' : '-',
				nFlags & TCP_FLAG_RESET  ? 'R' : '-',
				nFlags & TCP_FLAG_SYN    ? 'S' : '-',
				nFlags & TCP_FLAG_FIN    ? 'F' : '-',
				nSEG_SEQ-m_nIRS,
				nFlags & TCP_FLAG_ACK ? nSEG_ACK-m_nISS : 0,
				nSEG_WND,
				nDataLength);

	DumpStatus ();
#endif

	boolean bAcceptable = FALSE;

	// RFC 793 section 3.9 "SEGMENT ARRIVES"
	switch (m_State)
	{
	case TCPStateClosed:
		if (nFlags & TCP_FLAG_RESET)
		{
			// ignore
		}
		else if (!(nFlags & TCP_FLAG_ACK))
		{
			m_ForeignIP.Set (rSenderIP);
			m_nForeignPort = be2le16 (pHeader->nSourcePort);
			m_Checksum.SetDestinationAddress (rSenderIP);

			SendSegment (TCP_FLAG_RESET | TCP_FLAG_ACK, 0, nSEG_SEQ+nSEG_LEN);
		}
		else
		{
			m_ForeignIP.Set (rSenderIP);
			m_nForeignPort = be2le16 (pHeader->nSourcePort);
			m_Checksum.SetDestinationAddress (rSenderIP);

			SendSegment (TCP_FLAG_RESET, nSEG_ACK);
		}
		break;

	case TCPStateListen:
		if (nFlags & TCP_FLAG_RESET)
		{
			// ignore
		}
		else if (nFlags & TCP_FLAG_ACK)
		{
			m_ForeignIP.Set (rSenderIP);
			m_nForeignPort = be2le16 (pHeader->nSourcePort);
			m_Checksum.SetDestinationAddress (rSenderIP);

			SendSegment (TCP_FLAG_RESET, nSEG_ACK);
		}
		else if (nFlags & TCP_FLAG_SYN)
		{
			if (s_nConnections >= TCP_MAX_CONNECTIONS)
			{
				m_ForeignIP.Set (rSenderIP);
				m_nForeignPort = be2le16 (pHeader->nSourcePort);
				m_Checksum.SetDestinationAddress (rSenderIP);

				SendSegment (TCP_FLAG_RESET | TCP_FLAG_ACK, 0, nSEG_SEQ+nSEG_LEN);

				break;
			}

			m_nRCV_NXT = nSEG_SEQ+1;
			m_nIRS = nSEG_SEQ;

			m_nSND_WND = nSEG_WND;
			m_nSND_WL1 = nSEG_SEQ;
			m_nSND_WL2 = nSEG_ACK;
	
			assert (nSEG_LEN > 0);

			if (nDataLength > 0)
			{
				m_RxQueue.Enqueue ((u8 *) pPacket+nDataOffset, nDataLength);
			}

			m_nISS = CalculateISN ();
			m_RTOCalculator.Initialize (m_nISS);

			m_ForeignIP.Set (rSenderIP);
			m_nForeignPort = be2le16 (pHeader->nSourcePort);
			m_Checksum.SetDestinationAddress (rSenderIP);

			SendSegment (TCP_FLAG_SYN | TCP_FLAG_ACK, m_nISS, m_nRCV_NXT);
			m_RTOCalculator.SegmentSent (m_nISS);

			m_nSND_NXT = m_nISS+1;
			m_nSND_UNA = m_nISS;
			
			NEW_STATE (TCPStateSynReceived);

			m_Event.Set ();
		}
		break;

	case TCPStateSynSent:
		if (nFlags & TCP_FLAG_ACK)
		{
			if (!bwh (m_nISS, nSEG_ACK, m_nSND_NXT))
			{
				if (!(nFlags & TCP_FLAG_RESET))
				{
					SendSegment (TCP_FLAG_RESET, nSEG_ACK);
				}

				return 1;
			}
			else if (bwlh (m_nSND_UNA, nSEG_ACK, m_nSND_NXT))
			{
				bAcceptable = TRUE;
			}
		}

		if (nFlags & TCP_FLAG_RESET)
		{
			if (bAcceptable)
			{
				NEW_STATE (TCPStateClosed);
				m_bSendSYN = FALSE;
				m_nErrno = -1;

				m_Event.Set ();
			}
			
			break;
		}
		
		if (   (nFlags & TCP_FLAG_ACK)
		    && !bAcceptable)
		{
			break;
		}

		if (nFlags & TCP_FLAG_SYN)
		{
			m_nRCV_NXT = nSEG_SEQ+1;
			m_nIRS = nSEG_SEQ;

			if (nFlags & TCP_FLAG_ACK)
			{
				m_RTOCalculator.SegmentAcknowledged (nSEG_ACK);

				if (nSEG_ACK-m_nSND_UNA > 1)
				{
					m_RetransmissionQueue.Advance (nSEG_ACK-m_nSND_UNA-1);
				}

				m_nSND_UNA = nSEG_ACK;
			}

			if (gt (m_nSND_UNA, m_nISS))
			{
				NEW_STATE (TCPStateEstablished);
				m_bSendSYN = FALSE;

				StopTimer (TCPTimerRetransmission);

				// next transmission starts with this count
				m_nRetransmissionCount = MAX_RETRANSMISSIONS;

				m_Event.Set ();

				// RFC 1122 section 4.2.2.20 (c)
				m_nSND_WND = nSEG_WND;
				m_nSND_WL1 = nSEG_SEQ;
				m_nSND_WL2 = nSEG_ACK;
	
				SendSegment (TCP_FLAG_ACK, m_nSND_NXT, m_nRCV_NXT);
				
				if (   (nFlags & TCP_FLAG_FIN)		// other controls?
				    || nDataLength > 0)
				{
					goto StepSix;
				}

				break;
			}
			else
			{
				NEW_STATE (TCPStateSynReceived);
				m_bSendSYN = FALSE;
				SendSegment (TCP_FLAG_SYN | TCP_FLAG_ACK, m_nISS, m_nRCV_NXT);
				m_RTOCalculator.SegmentSent (m_nISS);
				m_nRetransmissionCount = MAX_RETRANSMISSIONS;
				StartTimer (TCPTimerRetransmission, m_RTOCalculator.GetRTO ());

				if (   (nFlags & TCP_FLAG_FIN)		// other controls?
				    || nDataLength > 0)
				{
					if (nFlags & TCP_FLAG_FIN)
					{
						SendSegment (TCP_FLAG_RESET, m_nSND_NXT);
						NEW_STATE (TCPStateClosed);
						m_nErrno = -1;
						m_Event.Set ();
					}

					if (nDataLength > 0)
					{
						m_RxQueue.Enqueue ((u8 *) pPacket+nDataOffset, nDataLength);
					}

					break;
				}
			}
		}
		break;

	case TCPStateSynReceived:
	case TCPStateEstablished:
	case TCPStateFinWait1:
	case TCPStateFinWait2:
	case TCPStateCloseWait:
	case TCPStateClosing:
	case TCPStateLastAck:
	case TCPStateTimeWait:
		// step 1 ( check sequence number)
		if (m_nRCV_WND > 0)
		{
			if (nSEG_LEN == 0)
			{
				if (bwl (m_nRCV_NXT, nSEG_SEQ, m_nRCV_NXT+m_nRCV_WND))
				{
					bAcceptable = TRUE;
				}
			}
			else
			{
				if (   bwl (m_nRCV_NXT, nSEG_SEQ, m_nRCV_NXT+m_nRCV_WND)
				    || bwl (m_nRCV_NXT, nSEG_SEQ+nSEG_LEN-1, m_nRCV_NXT+m_nRCV_WND))
				{
					bAcceptable = TRUE;
				}
			}
		}
		else
		{
			if (nSEG_LEN == 0)
			{
				if (nSEG_SEQ == m_nRCV_NXT)
				{
					bAcceptable = TRUE;
				}
			}
		}

		if (   !bAcceptable
		    && m_State != TCPStateSynReceived)
		{
			SendSegment (TCP_FLAG_ACK, m_nSND_NXT, m_nRCV_NXT);
			break;
		}

		// step 2 (check RST bit)
		if (nFlags & TCP_FLAG_RESET)
		{
			switch (m_State)
			{
			case TCPStateSynReceived:
				m_RetransmissionQueue.Flush ();
				if (!m_bActiveOpen)
				{
					NEW_STATE (TCPStateListen);
					return 1;
				}
				else
				{
					m_nErrno = -1;
					NEW_STATE (TCPStateClosed);
					m_Event.Set ();
					return 1;
					
				}
				break;

			case TCPStateEstablished:
			case TCPStateFinWait1:
			case TCPStateFinWait2:
			case TCPStateCloseWait:
				m_nErrno = -1;
				m_RetransmissionQueue.Flush ();
				m_TxQueue.Flush ();
				m_RxQueue.Flush ();
				NEW_STATE (TCPStateClosed);
				m_Event.Set ();
				return 1;

			case TCPStateClosing:
			case TCPStateLastAck:
			case TCPStateTimeWait:
				NEW_STATE (TCPStateClosed);
				m_Event.Set ();
				return 1;

			default:
				UNEXPECTED_STATE ();
				return 1;
			}
		}

		// step 3 (check security and precedence, not supported)
		
		// step 4 (check SYN bit)
		if (nFlags & TCP_FLAG_SYN)
		{
			// RFC 1122 section 4.2.2.20 (e)
			if (   m_State == TCPStateSynReceived
			    && !m_bActiveOpen)
			{
				NEW_STATE (TCPStateListen);
				return 1;
			}
			
			SendSegment (TCP_FLAG_RESET, m_nSND_NXT);
			m_nErrno = -1;
			m_RetransmissionQueue.Flush ();
			m_TxQueue.Flush ();
			m_RxQueue.Flush ();
			NEW_STATE (TCPStateClosed);
			m_Event.Set ();
			return 1;
		}

		// step 5 (check ACK field)
		if (!(nFlags & TCP_FLAG_ACK))
		{
			return 1;
		}

		switch (m_State)
		{
		case TCPStateSynReceived:
			if (bwlh (m_nSND_UNA, nSEG_ACK, m_nSND_NXT))
			{
				// RFC 1122 section 4.2.2.20 (f)
				m_nSND_WND = nSEG_WND;
				m_nSND_WL1 = nSEG_SEQ;
				m_nSND_WL2 = nSEG_ACK;

				m_nSND_UNA = nSEG_ACK;		// got ACK for SYN

				m_RTOCalculator.SegmentAcknowledged (nSEG_ACK);

				NEW_STATE (TCPStateEstablished);

				// next transmission starts with this count
				m_nRetransmissionCount = MAX_RETRANSMISSIONS;
			}
			else
			{
				SendSegment (TCP_FLAG_RESET, nSEG_ACK);
			}
			break;

		case TCPStateEstablished:
		case TCPStateFinWait1:
		case TCPStateFinWait2:
		case TCPStateCloseWait:
		case TCPStateClosing:
			if (bwh (m_nSND_UNA, nSEG_ACK, m_nSND_NXT))
			{
				m_RTOCalculator.SegmentAcknowledged (nSEG_ACK);

				unsigned nBytesAck = nSEG_ACK-m_nSND_UNA;
				m_nSND_UNA = nSEG_ACK;

				if (nSEG_ACK == m_nSND_NXT)	// all segments are acknowledged
				{
					StopTimer (TCPTimerRetransmission);

					// next transmission starts with this count
					m_nRetransmissionCount = MAX_RETRANSMISSIONS;
				}

				if (   m_State == TCPStateFinWait1
				    || m_State == TCPStateClosing)
				{
					nBytesAck--;		// acknowledged FIN does not count
					m_bFINQueued = FALSE;
				}

				if (   m_State == TCPStateEstablished
				    && nBytesAck == 1)
				{
					nBytesAck--;
				}
				
				if (nBytesAck > 0)
				{
					m_RetransmissionQueue.Advance (nBytesAck);
				}

				// update send window
				if (   lt (m_nSND_WL1, nSEG_SEQ)
				    || (   m_nSND_WL1 == nSEG_SEQ
				        && le (m_nSND_WL2, nSEG_ACK)))
				{
					m_nSND_WND = nSEG_WND;
					m_nSND_WL1 = nSEG_SEQ;
					m_nSND_WL2 = nSEG_ACK;
				}
			}
			else if (le (nSEG_ACK, m_nSND_UNA))	// RFC 1122 section 4.2.2.20 (g)
			{
				// ignore duplicate ACK ...
				
				// RFC 1122 section 4.2.2.20 (g)
				if (bwlh (m_nSND_UNA, nSEG_ACK, m_nSND_NXT))
				{
					// ... but update send window
					if (   lt (m_nSND_WL1, nSEG_SEQ)
					    || (   m_nSND_WL1 == nSEG_SEQ
						&& le (m_nSND_WL2, nSEG_ACK)))
					{
						m_nSND_WND = nSEG_WND;
						m_nSND_WL1 = nSEG_SEQ;
						m_nSND_WL2 = nSEG_ACK;
					}
				}
			}
			else if (gt (nSEG_ACK, m_nSND_NXT))
			{
				SendSegment (TCP_FLAG_ACK, m_nSND_NXT, m_nRCV_NXT);
				return 1;
			}
			
			switch (m_State)
			{
			case TCPStateEstablished:
			case TCPStateCloseWait:
				break;

			case TCPStateFinWait1:
				if (nSEG_ACK == m_nSND_NXT)	// if our FIN is now acknowledged
				{
					m_RTOCalculator.SegmentAcknowledged (nSEG_ACK);

					m_bFINQueued = FALSE;
					StopTimer (TCPTimerRetransmission);
					NEW_STATE (TCPStateFinWait2);
					StartTimer (TCPTimerTimeWait, HZ_FIN_TIMEOUT);
				}
				else
				{
					break;
				}
				// fall through

			case TCPStateFinWait2:
				if (m_RetransmissionQueue.IsEmpty ())
				{
					m_Event.Set ();
				}
				break;
				
			case TCPStateClosing:
				if (nSEG_ACK == m_nSND_NXT)	// if our FIN is now acknowledged
				{
					m_RTOCalculator.SegmentAcknowledged (nSEG_ACK);

					m_bFINQueued = FALSE;
					StopTimer (TCPTimerRetransmission);
					NEW_STATE (TCPStateTimeWait);
					StartTimer (TCPTimerTimeWait, HZ_TIMEWAIT);
				}
				break;

			default:
				UNEXPECTED_STATE ();
				break;
			}
			break;
			
		case TCPStateLastAck:
			if (nSEG_ACK == m_nSND_NXT)	// if our FIN is now acknowledged
			{
				m_bFINQueued = FALSE;
				NEW_STATE (TCPStateClosed);
				m_Event.Set ();
				return 1;
			}
			break;

		case TCPStateTimeWait:
			if (nSEG_ACK == m_nSND_NXT)	// if our FIN is now acknowledged
			{
				m_bFINQueued = FALSE;
				SendSegment (TCP_FLAG_ACK, m_nSND_NXT, m_nRCV_NXT);
				StartTimer (TCPTimerTimeWait, HZ_TIMEWAIT);
			}
			break;

		default:
			UNEXPECTED_STATE ();
			break;
		}

		// step 6 (check URG bit, not supported)
	StepSix:

		// step 7 (process text segment)
		if (nSEG_LEN == 0)
		{
			return 1;
		}
		
		switch (m_State)
		{
		case TCPStateEstablished:
		case TCPStateFinWait1:
		case TCPStateFinWait2:
			if (nSEG_SEQ == m_nRCV_NXT)
			{
				if (nDataLength > 0)
				{
					m_RxQueue.Enqueue ((u8 *) pPacket+nDataOffset, nDataLength);

					m_nRCV_NXT += nDataLength;

					// m_nRCV_WND should be adjusted here (section 3.7)

					// following ACK could be piggybacked with data
					SendSegment (TCP_FLAG_ACK, m_nSND_NXT, m_nRCV_NXT);

					if (nFlags & TCP_FLAG_PUSH)
					{
						m_Event.Set ();
					}
				}
			}
			else
			{
				SendSegment (TCP_FLAG_ACK, m_nSND_NXT, m_nRCV_NXT);
				return 1;
			}
			break;

		case TCPStateSynReceived:	// this state not in RFC 793
		case TCPStateCloseWait:
		case TCPStateClosing:
		case TCPStateLastAck:
		case TCPStateTimeWait:
			break;

		default:
			UNEXPECTED_STATE ();
			break;
		}

		// step 8 (check FIN bit)
		if (   m_State == TCPStateClosed
		    || m_State == TCPStateListen
		    || m_State == TCPStateSynSent)
		{
			return 1;
		}
			
		if (!(nFlags & TCP_FLAG_FIN))
		{
			return 1;
		}

		// connection is closing
		m_nRCV_NXT++;
		SendSegment (TCP_FLAG_ACK, m_nSND_NXT, m_nRCV_NXT);
		
		switch (m_State)
		{
		case TCPStateSynReceived:
		case TCPStateEstablished:
			NEW_STATE (TCPStateCloseWait);
			m_Event.Set ();
			break;

		case TCPStateFinWait1:
			if (nSEG_ACK == m_nSND_NXT)	// if our FIN is now acknowledged
			{
				m_bFINQueued = FALSE;
				StopTimer (TCPTimerRetransmission);
				StopTimer (TCPTimerUser);
				NEW_STATE (TCPStateTimeWait);
				StartTimer (TCPTimerTimeWait, HZ_TIMEWAIT);
			}
			else
			{
				NEW_STATE (TCPStateClosing);
			}
			break;

		case TCPStateFinWait2:
			StopTimer (TCPTimerRetransmission);
			StopTimer (TCPTimerUser);
			NEW_STATE (TCPStateTimeWait);
			StartTimer (TCPTimerTimeWait, HZ_TIMEWAIT);
			break;

		case TCPStateCloseWait:
		case TCPStateClosing:
		case TCPStateLastAck:
			break;
			
		case TCPStateTimeWait:
			StartTimer (TCPTimerTimeWait, HZ_TIMEWAIT);
			break;

		default:
			UNEXPECTED_STATE ();
			break;
		}
		break;
	}

	return 1;
}

int CTCPConnection::NotificationReceived (TICMPNotificationType  Type,
					  CIPAddress		&rSenderIP,
					  CIPAddress		&rReceiverIP,
					  u16			 nSendPort,
					  u16			 nReceivePort,
					  int			 nProtocol)
{
	if (nProtocol != IPPROTO_TCP)
	{
		return 0;
	}

	if (m_State < TCPStateSynSent)
	{
		return 0;
	}

	if (   m_ForeignIP != rSenderIP
	    || m_nForeignPort != nSendPort)
	{
		return 0;
	}

	assert (m_pNetConfig != 0);
	if (   rReceiverIP != *m_pNetConfig->GetIPAddress ()
	    || m_nOwnPort != nReceivePort)
	{
		return 0;
	}

	m_nErrno = -1;

	StopTimer (TCPTimerRetransmission);
	NEW_STATE (TCPStateTimeWait);
	StartTimer (TCPTimerTimeWait, HZ_TIMEWAIT);

	m_Event.Set ();

	return 1;
}

boolean CTCPConnection::SendSegment (unsigned nFlags, u32 nSequenceNumber, u32 nAcknowledgmentNumber,
				     const void *pData, unsigned nDataLength)
{
	unsigned nDataOffset = 5;
	assert (nDataOffset * 4 == sizeof (TTCPHeader));
	if (nFlags & TCP_FLAG_SYN)
	{
		nDataOffset++;
	}
	unsigned nHeaderLength = nDataOffset * 4;
	
	unsigned nPacketLength = nHeaderLength + nDataLength;		// may wrap
	assert (nPacketLength >= nHeaderLength);
	assert (nHeaderLength <= FRAME_BUFFER_SIZE);

	u8 TxBuffer[FRAME_BUFFER_SIZE];
	TTCPHeader *pHeader = (TTCPHeader *) TxBuffer;

	pHeader->nSourcePort	 	= le2be16 (m_nOwnPort);
	pHeader->nDestPort	 	= le2be16 (m_nForeignPort);
	pHeader->nSequenceNumber 	= le2be32 (nSequenceNumber);
	pHeader->nAcknowledgmentNumber	= nFlags & TCP_FLAG_ACK ? le2be32 (nAcknowledgmentNumber) : 0;
	pHeader->nDataOffsetFlags	= (nDataOffset << TCP_DATA_OFFSET_SHIFT) | nFlags;
	pHeader->nWindow		= le2be16 (m_nRCV_WND);
	pHeader->nUrgentPointer		= le2be16 (m_nSND_UP);

	if (nFlags & TCP_FLAG_SYN)
	{
		TTCPOption *pOption = (TTCPOption *) pHeader->Options;

		pOption->nKind   = TCP_OPTION_MSS;
		pOption->nLength = 4;
		pOption->Data[0] = TCP_CONFIG_MSS >> 8;
		pOption->Data[1] = TCP_CONFIG_MSS & 0xFF;
	}

	if (nDataLength > 0)
	{
		assert (pData != 0);
		memcpy (TxBuffer+nHeaderLength, pData, nDataLength);
	}

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
				nSequenceNumber-m_nISS,
				nFlags & TCP_FLAG_ACK ? nAcknowledgmentNumber-m_nIRS : 0,
				m_nRCV_WND,
				nDataLength);
#endif

	assert (m_pNetworkLayer != 0);
	return m_pNetworkLayer->Send (m_ForeignIP, TxBuffer, nPacketLength, IPPROTO_TCP);
}

void CTCPConnection::ScanOptions (TTCPHeader *pHeader)
{
	assert (pHeader != 0);
	unsigned nDataOffset = TCP_DATA_OFFSET (pHeader->nDataOffsetFlags)*4;
	u8 *pHeaderEnd = (u8 *) pHeader+nDataOffset;

	TTCPOption *pOption = (TTCPOption *) pHeader->Options;
	while ((u8 *) pOption+2 <= pHeaderEnd)
	{
		switch (pOption->nKind)
		{
		case TCP_OPTION_END_OF_LIST:
			return;

		case TCP_OPTION_NOP:
			pOption = (TTCPOption *) ((u8 *) pOption+1);
			break;
			
		case TCP_OPTION_MSS:
			if (   pOption->nLength == 4
			    && (u8 *) pOption+4 <= pHeaderEnd)
			{
				u32 nMSS = (u16) pOption->Data[0] << 8 | pOption->Data[1];

				// RFC 1122 section 4.2.2.6
				nMSS = min (nMSS+20, MSS_S) - TCP_HEADER_SIZE - IP_OPTION_SIZE;

				if (nMSS >= 10)		// self provided sanity check
				{
					m_nSND_MSS = (u16) nMSS;
				}
			}
			// fall through

		default:
			pOption = (TTCPOption *) ((u8 *) pOption+pOption->nLength);
			break;
		}
	}
}

u32 CTCPConnection::CalculateISN (void)
{
	assert (m_pTimer != 0);
	return   (  m_pTimer->GetTime () * HZ
	          + m_pTimer->GetTicks () % HZ)
	       * (TCP_MAX_WINDOW / TCP_QUIET_TIME / HZ);
}

void CTCPConnection::StartTimer (unsigned nTimer, unsigned nHZ)
{
	assert (nTimer < TCPTimerUnknown);
	assert (nHZ > 0);
	assert (m_pTimer != 0);

	StopTimer (nTimer);

	m_hTimer[nTimer] = m_pTimer->StartKernelTimer (nHZ, TimerStub, (void *) (uintptr) nTimer, this);
}

void CTCPConnection::StopTimer (unsigned nTimer)
{
	assert (nTimer < TCPTimerUnknown);
	assert (m_pTimer != 0);

	m_TimerSpinLock.Acquire ();

	if (m_hTimer[nTimer] != 0)
	{
		m_pTimer->CancelKernelTimer (m_hTimer[nTimer]);
		m_hTimer[nTimer] = 0;
	}

	m_TimerSpinLock.Release ();
}

void CTCPConnection::TimerHandler (unsigned nTimer)
{
	assert (nTimer < TCPTimerUnknown);

	m_TimerSpinLock.Acquire ();

	if (m_hTimer[nTimer] == 0)		// timer was stopped in the meantime
	{
		m_TimerSpinLock.Release ();

		return;
	}

	m_hTimer[nTimer] = 0;

	m_TimerSpinLock.Release ();

	switch (nTimer)
	{
	case TCPTimerRetransmission:
		m_RTOCalculator.RetransmissionTimerExpired ();

		if (m_nRetransmissionCount-- == 0)
		{
			m_bTimedOut = TRUE;
			break;
		}

		switch (m_State)
		{
		case TCPStateClosed:
		case TCPStateListen:
		case TCPStateFinWait2:
		case TCPStateTimeWait:
			UNEXPECTED_STATE ();
			break;

		case TCPStateSynSent:
		case TCPStateSynReceived:
			assert (!m_bSendSYN);
			m_bSendSYN = TRUE;
			break;

		case TCPStateEstablished:
		case TCPStateCloseWait:
			assert (!m_bRetransmit);
			m_bRetransmit = TRUE;
			break;

		case TCPStateFinWait1:
		case TCPStateClosing:
		case TCPStateLastAck:
			assert (!m_bFINQueued);
			m_bFINQueued = TRUE;
			break;
		}
		break;

	case TCPTimerTimeWait:
		NEW_STATE (TCPStateClosed);
		break;

	case TCPTimerUser:
	case TCPTimerUnknown:
		assert (0);
		break;
	}
}

void CTCPConnection::TimerStub (TKernelTimerHandle hTimer, void *pParam, void *pContext)
{
	CTCPConnection *pThis = (CTCPConnection *) pContext;
	assert (pThis != 0);

	unsigned nTimer = (unsigned) (uintptr) pParam;
	assert (nTimer < TCPTimerUnknown);

	pThis->TimerHandler (nTimer);
}

#ifndef NDEBUG

void CTCPConnection::DumpStatus (void)
{
	CLogger::Get ()->Write (FromTCP, LogDebug,
				"sta %u, una %u, snx %u, swn %u, rnx %u, rwn %u, fprt %u",
				m_State,
				m_nSND_UNA-m_nISS,
				m_nSND_NXT-m_nISS,
				m_nSND_WND,
				m_nRCV_NXT-m_nIRS,
				m_nRCV_WND,
				(unsigned) m_nForeignPort);
}

TTCPState CTCPConnection::NewState (TTCPState State, unsigned nLine)
{
	const static char *StateName[] =	// must match TTCPState
	{
		"CLOSED",
		"LISTEN",
		"SYN-SENT",
		"SYN-RECEIVED",
		"ESTABLISHED",
		"FIN-WAIT-1",
		"FIN-WAIT-2",
		"CLOSE-WAIT",
		"CLOSING",
		"LAST-ACK",
		"TIME-WAIT"
	};

	assert (m_State < sizeof StateName / sizeof StateName[0]);
	assert (State < sizeof StateName / sizeof StateName[0]);

	CLogger::Get ()->Write (FromTCP, LogDebug, "State %s -> %s at line %u", StateName[m_State], StateName[State], nLine);

	return m_State = State;
}

void CTCPConnection::UnexpectedState (unsigned nLine)
{
	DumpStatus ();

	CLogger::Get ()->Write (FromTCP, LogPanic, "Unexpected state %u at line %u", m_State, nLine);
}

#endif
