//
// retranstimeoutcalc.h
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
#ifndef _circle_net_retranstimeoutcalc_h
#define _circle_net_retranstimeoutcalc_h

#include <circle/timer.h>
#include <circle/spinlock.h>
#include <circle/types.h>

class CRetransmissionTimeoutCalculator					// all times in HZ units
{
public:
	CRetransmissionTimeoutCalculator (void);
	~CRetransmissionTimeoutCalculator (void);

	unsigned GetRTO (void) const;

	void Initialize (u32 nISN, unsigned nMaxWindow, unsigned nMSS);

	void SegmentSent (u32 nSequenceNumber, u32 nLength = 1);
	void SegmentAcknowledged (u32 nAcknowledgmentNumber);

	void RetransmissionTimerExpired (u32 nSegmentNumberExpected);

private:
	void Calculate (unsigned nRTT);

private:
	struct TSegmentInfo
	{
		boolean bUsed;
		u32 nSequenceNumber;
		unsigned nStartTicks;
		unsigned nRetransmissions;
	};

	static const unsigned MaxWindow = 65535;
	static const unsigned MinMSS = 512;
	static const unsigned SegmentMapSize = MaxWindow / (MinMSS / 2) + 1;

	unsigned CalculateHash (u32 nSequenceNumber)
	{
		return (nSequenceNumber % m_nMaxWindow) / (m_nMSS / 2);
	}

private:
	CTimer *m_pTimer;

	u32 m_nISN;
	unsigned m_nMaxWindow;
	unsigned m_nMSS;
	unsigned m_nRTO;

	boolean m_bFirstMeasurement;
	unsigned m_nSRTT;
	unsigned m_nRTTVAR;

	TSegmentInfo m_SegmentMap[SegmentMapSize];

	CSpinLock m_SpinLock;
};

#endif
