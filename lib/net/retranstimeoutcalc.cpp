//
// retranstimeoutcalc.cpp
//
// Calculating TCP retransmission timeout according to RFC 6298
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2026  R. Stange <rsta2@gmx.net>
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
#include <circle/net/retranstimeoutcalc.h>
#include <circle/logger.h>
#include <assert.h>

//#define RTO_DEBUG

#define INITIAL_RTO		(3 * HZ)	// cannot return to 3 secs on data phase, so start with it
#define MIN_RTO			(1 * HZ)
#define MAX_RTO			(120 * HZ)

#define CLEAR_SRTT_AFTER	3		// retransmissions

#define ALPHA			8
#define BETA			4
#define K			4
#define G			1		// timer granularity in 1 HZ

#ifdef RTO_DEBUG
static const char FromRTO[] = "tcprto";
#endif

CRetransmissionTimeoutCalculator::CRetransmissionTimeoutCalculator (void)
:	m_pTimer (CTimer::Get ()),
	m_nISN (0),
	m_nMaxWindow (0),
	m_nMinMSS (0),
	m_nRTO (INITIAL_RTO),
	m_bFirstMeasurement (TRUE),
	m_nSegmentMapSize (0),
	m_SegmentMap (0)
{
	assert (m_pTimer != 0);
}

CRetransmissionTimeoutCalculator::~CRetransmissionTimeoutCalculator (void)
{
	delete m_SegmentMap;
	m_SegmentMap = 0;

	m_pTimer = 0;
}

unsigned CRetransmissionTimeoutCalculator::GetRTO (void) const
{
	return m_nRTO;
}

void CRetransmissionTimeoutCalculator::Initialize (u32 nISN, unsigned nMaxWindow, unsigned nMinMSS)
{
	assert (nMinMSS >= 2);
	assert (nMinMSS <= nMaxWindow);
	m_nSegmentMapSize = nMaxWindow / (nMinMSS / 2) + 1;
	m_SegmentMap = new TSegmentInfo[m_nSegmentMapSize];
	assert (m_SegmentMap != 0);

	m_SpinLock.Acquire ();

#ifdef RTO_DEBUG
	CLogger::Get ()->Write (FromRTO, LogDebug, "Initial sequence number is %u", nISN);
#endif

	m_nISN = nISN;
	m_nMaxWindow = nMaxWindow;
	m_nMinMSS = nMinMSS;
	m_nRTO = INITIAL_RTO;
	m_bFirstMeasurement = TRUE;

	for (unsigned i = 0; i < m_nSegmentMapSize; i++)
	{
		m_SegmentMap[i].bUsed = FALSE;
	}

	m_SpinLock.Release ();
}

void CRetransmissionTimeoutCalculator::SegmentSent (u32 nSequenceNumber, u32 nLength)
{
	unsigned nHash = CalculateHash (nSequenceNumber);
	assert (nHash < m_nSegmentMapSize);
	TSegmentInfo &Info = m_SegmentMap[nHash];

	m_SpinLock.Acquire ();

	if (   Info.bUsed
	    && Info.nSequenceNumber == nSequenceNumber)
	{
		Info.nRetransmissions++;
	}
	else
	{
		assert (m_pTimer != 0);

		Info.bUsed = TRUE;
		Info.nSequenceNumber = nSequenceNumber;
		Info.nStartTicks = m_pTimer->GetTicks ();
		Info.nRetransmissions = 0;
	}

#ifdef RTO_DEBUG
	CLogger::Get ()->Write (FromRTO, LogDebug, "Segment sent (seq %u, len %u, retrans %u)",
				nSequenceNumber-m_nISN, nLength, Info.nRetransmissions);
#endif

	m_SpinLock.Release ();
}

void CRetransmissionTimeoutCalculator::SegmentAcknowledged (u32 nAcknowledgmentNumber)
{
	unsigned nHash = CalculateHash (nAcknowledgmentNumber);
	assert (nHash < m_nSegmentMapSize);
	TSegmentInfo &Info = m_SegmentMap[nHash];

	m_SpinLock.Acquire ();

#ifdef RTO_DEBUG
	CLogger::Get ()->Write (FromRTO, LogDebug, "Segment acknowledged (ack %u)",
				nAcknowledgmentNumber-m_nISN);
#endif

	if (   Info.bUsed
	    && Info.nSequenceNumber == nAcknowledgmentNumber
	    && Info.nRetransmissions == 0)
	{
		assert (m_pTimer != 0);
		unsigned nRTT = m_pTimer->GetTicks () - Info.nStartTicks;
#ifdef RTO_DEBUG
		CLogger::Get ()->Write (FromRTO, LogDebug, "RTT was %u", nRTT);
#endif

		Calculate (nRTT);
	}

	m_SpinLock.Release ();
}

void CRetransmissionTimeoutCalculator::RetransmissionTimerExpired (u32 nSegmentNumberExpected)
{
	unsigned nHash = CalculateHash (nSegmentNumberExpected);
	assert (nHash < m_nSegmentMapSize);
	TSegmentInfo &Info = m_SegmentMap[nHash];

	m_SpinLock.Acquire ();

	m_nRTO *= 2;					// back off
	if (m_nRTO > MAX_RTO)
	{
		m_nRTO = MAX_RTO;
	}

#ifdef RTO_DEBUG
	CLogger::Get ()->Write (FromRTO, LogDebug, "New RTO is %u", m_nRTO);
#endif

	if (   Info.bUsed
	    && Info.nSequenceNumber == nSegmentNumberExpected)
	{
		if (++Info.nRetransmissions >= CLEAR_SRTT_AFTER)	// RFC 6298 note 5.7
		{
			m_bFirstMeasurement = TRUE;
		}
	}

#ifdef RTO_DEBUG
	CLogger::Get ()->Write (FromRTO, LogDebug, "Retransmission #%u (seq %u)",
				Info.nRetransmissions, nSegmentNumberExpected-m_nISN);
#endif

	m_SpinLock.Release ();
}

void CRetransmissionTimeoutCalculator::Calculate (unsigned nRTT)
{
	if (m_bFirstMeasurement)
	{
		m_bFirstMeasurement = FALSE;

		// RFC 6298 2.2
		m_nSRTT = nRTT;
		m_nRTTVAR = nRTT / 2;
	}
	else
	{
		// RFC 6298 2.3
		int nRTTDiff = m_nSRTT - nRTT;
		if (nRTTDiff < 0)
		{
			nRTTDiff = -nRTTDiff;
		}
		
		m_nRTTVAR = ((BETA - 1) * m_nRTTVAR + nRTTDiff) / BETA;
		m_nSRTT = ((ALPHA - 1) * m_nSRTT + nRTT) / ALPHA;
	}

	unsigned nMax = K * m_nRTTVAR;
	if (nMax < G)
	{
		nMax = G;
	}
	m_nRTO = m_nSRTT + nMax;

	if (m_nRTO < MIN_RTO)
	{
		m_nRTO = MIN_RTO;
	}
	else if (m_nRTO > MAX_RTO)
	{
		m_nRTO = MAX_RTO;
	}

#ifdef RTO_DEBUG
	CLogger::Get ()->Write (FromRTO, LogDebug, "New RTO is %u (srtt %u, rttvar %u)",
				m_nRTO, m_nSRTT, m_nRTTVAR);
#endif
}
