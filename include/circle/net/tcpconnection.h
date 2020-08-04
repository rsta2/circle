//
// tcpconnection.h
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
#ifndef _circle_net_tcpconnection_h
#define _circle_net_tcpconnection_h

#include <circle/net/netconnection.h>
#include <circle/net/networklayer.h>
#include <circle/net/ipaddress.h>
#include <circle/net/icmphandler.h>
#include <circle/net/netqueue.h>
#include <circle/net/retransmissionqueue.h>
#include <circle/net/retranstimeoutcalc.h>
#include <circle/sched/synchronizationevent.h>
#include <circle/timer.h>
#include <circle/spinlock.h>
#include <circle/types.h>

enum TTCPState
{
	TCPStateClosed,
	TCPStateListen,
	TCPStateSynSent,
	TCPStateSynReceived,
	TCPStateEstablished,
	TCPStateFinWait1,
	TCPStateFinWait2,
	TCPStateCloseWait,
	TCPStateClosing,
	TCPStateLastAck,
	TCPStateTimeWait
};

enum TTCPTimer
{
	TCPTimerUser,
	TCPTimerRetransmission,
	TCPTimerTimeWait,
	TCPTimerUnknown
};

struct TTCPHeader;

class CTCPConnection : public CNetConnection
{
public:
	CTCPConnection (CNetConfig	*pNetConfig,		// active OPEN
			CNetworkLayer	*pNetworkLayer,
			CIPAddress	&rForeignIP,
			u16		 nForeignPort,
			u16		 nOwnPort);
	CTCPConnection (CNetConfig	*pNetConfig,		// passive OPEN
			CNetworkLayer	*pNetworkLayer,
			u16		 nOwnPort);
	~CTCPConnection (void);

	int Connect (void);
	int Accept (CIPAddress *pForeignIP, u16 *pForeignPort);
	int Close (void);
	
	int Send (const void *pData, unsigned nLength, int nFlags);
	int Receive (void *pBuffer, int nFlags);

	int SendTo (const void *pData, unsigned nLength, int nFlags, CIPAddress	&rForeignIP, u16 nForeignPort);
	int ReceiveFrom (void *pBuffer, int nFlags, CIPAddress *pForeignIP, u16 *pForeignPort);

	int SetOptionBroadcast (boolean bAllowed);

	boolean IsConnected (void) const;
	boolean IsTerminated (void) const;
	
	void Process (void);
	
	// returns: -1: invalid packet, 0: not to me, 1: packet consumed
	int PacketReceived (const void *pPacket, unsigned nLength,
			    CIPAddress &rSenderIP, CIPAddress &rReceiverIP, int nProtocol);

	// returns: 0: not to me, 1: notification consumed
	int NotificationReceived (TICMPNotificationType Type,
				  CIPAddress &rSenderIP, CIPAddress &rReceiverIP,
				  u16 nSendPort, u16 nReceivePort,
				  int nProtocol);

private:
	boolean SendSegment (unsigned nFlags, u32 nSequenceNumber, u32 nAcknowledgmentNumber = 0,
			     const void *pData = 0, unsigned nDataLength = 0);

	void ScanOptions (TTCPHeader *pHeader);
	
	u32 CalculateISN (void);
	
	void StartTimer (unsigned nTimer, unsigned nHZ);
	void StopTimer (unsigned nTimer);
	void TimerHandler (unsigned nTimer);
	static void TimerStub (TKernelTimerHandle hTimer, void *pParam, void *pContext);

#ifndef NDEBUG
	void DumpStatus (void);
	TTCPState NewState (TTCPState State, unsigned nLine);
	void UnexpectedState (unsigned nLine);
#endif

private:
	boolean m_bActiveOpen;
	volatile TTCPState m_State;

	volatile int m_nErrno;			// signalize error to the user

	CNetQueue m_TxQueue;
	CNetQueue m_RxQueue;

	CRetransmissionQueue m_RetransmissionQueue;
	volatile boolean m_bRetransmit;		// reset m_RetransmissionQueue and send
	volatile boolean m_bSendSYN;		// send SYN when in TCPStateSynSent or TCPStateSynReceived
	volatile boolean m_bFINQueued;		// send FIN when TX and retransmission queues are empty
	TTCPState m_StateAfterFIN;		//	and go to this state

	volatile unsigned m_nRetransmissionCount;
	volatile boolean m_bTimedOut;		// abort connection and close
	
	CSynchronizationEvent m_Event;
	CSynchronizationEvent m_TxEvent;	// for pacing transmit

	CTimer *m_pTimer;
	TKernelTimerHandle m_hTimer[TCPTimerUnknown];
	CSpinLock m_TimerSpinLock;

	// Send Sequence Variables
	u32 m_nSND_UNA;		// send unacknowledged
	u32 m_nSND_NXT;		// send next
	u32 m_nSND_WND;		// send window
	u16 m_nSND_UP;		// send urgent pointer
	u32 m_nSND_WL1;		// segment sequence number used for last window update
	u32 m_nSND_WL2;		// segment acknowledgment number used for last window update
	u32 m_nISS;		// initial send sequence number

	// Receive Sequence Variables
	u32 m_nRCV_NXT;		// receive next
	u32 m_nRCV_WND;		// receive window
	//u16 m_nRCV_UP;	// receive urgent pointer
	u32 m_nIRS;		// initial receive sequence number

	// Other Variables
	u16 m_nSND_MSS;		// send maximum segment size

	CRetransmissionTimeoutCalculator m_RTOCalculator;

	static unsigned s_nConnections;
};

#endif
