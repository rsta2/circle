//
// retranstimeoutcalc.h
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

	void Initialize (u32 nISN);

	void SegmentSent (u32 nSequenceNumber, u32 nLength = 1);
	void SegmentAcknowledged (u32 nAcknowledgmentNumber);		// called for valid ACKs only

	void RetransmissionTimerExpired (void);

private:
	void Calculate (unsigned nRTT);

private:
	CTimer *m_pTimer;

	u32 m_nISN;
	unsigned m_nRTO;

	boolean m_bFirstMeasurement;
	unsigned m_nSRTT;
	unsigned m_nRTTVAR;

	boolean m_bMeasurementRuns;
	unsigned m_nStartTicks;
	unsigned m_nRetransmissions;

	CSpinLock m_SpinLock;
};

#endif
