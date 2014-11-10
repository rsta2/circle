//
// dwhciframeschedper.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/dwhciframeschedper.h>
#include <circle/usb/dwhciregister.h>
#include <circle/usb/dwhci.h>
#include <circle/logger.h>
#include <assert.h>

#define uFRAME			125		// micro seconds

#define FRAME_UNSET		8

enum TFrameSchedulerState
{
	StateStartSplit,
	StateStartSplitComplete,
	StateCompleteSplit,
	StateCompleteRetry,
	StateCompleteSplitComplete,
	StateCompleteSplitFailed,
	StateUnknown
};

CDWHCIFrameSchedulerPeriodic::CDWHCIFrameSchedulerPeriodic (void)
:	m_pTimer (CTimer::Get ()),
	m_nState (StateUnknown),
	m_nNextFrame (FRAME_UNSET)
{
	assert (m_pTimer != 0);
}

CDWHCIFrameSchedulerPeriodic::~CDWHCIFrameSchedulerPeriodic (void)
{
	m_pTimer = 0;

	m_nState = StateUnknown;
}

void CDWHCIFrameSchedulerPeriodic::StartSplit (void)
{
	m_nState = StateStartSplit;
	m_nNextFrame = FRAME_UNSET;
}

boolean CDWHCIFrameSchedulerPeriodic::CompleteSplit (void)
{
	boolean bResult = FALSE;
	
	switch (m_nState)
	{
	case StateStartSplitComplete:
		m_nState = StateCompleteSplit;
		m_nTries = m_nNextFrame != 5 ? 3 : 2;
		m_nNextFrame = (m_nNextFrame  + 2) & 7;
		bResult = TRUE;
		break;

	case StateCompleteRetry:
		bResult = TRUE;
		m_nNextFrame = (m_nNextFrame + 1) & 7;
		break;

	case StateCompleteSplitComplete:
	case StateCompleteSplitFailed:
		break;
		
	default:
		assert (0);
		break;
	}
	
	return bResult;
}

void CDWHCIFrameSchedulerPeriodic::TransactionComplete (u32 nStatus)
{
	switch (m_nState)
	{
	case StateStartSplit:
		assert (nStatus & DWHCI_HOST_CHAN_INT_ACK);
		m_nState = StateStartSplitComplete;
		break;

	case StateCompleteSplit:
	case StateCompleteRetry:
		if (nStatus & DWHCI_HOST_CHAN_INT_XFER_COMPLETE)
		{
			m_nState = StateCompleteSplitComplete;
		}
		else if (nStatus & (DWHCI_HOST_CHAN_INT_NYET | DWHCI_HOST_CHAN_INT_ACK))
		{
			if (m_nTries-- == 0)
			{
				m_nState = StateCompleteSplitFailed;

				m_pTimer->usDelay (8 * uFRAME);
			}
			else
			{
				m_nState = StateCompleteRetry;
			}
		}
		else if (nStatus & DWHCI_HOST_CHAN_INT_NAK)
		{
			m_pTimer->usDelay (5 * uFRAME);
			m_nState = StateCompleteSplitFailed;
		}
		else
		{
			CLogger::Get ()->Write ("dwsched", LogError, "Invalid status 0x%X", nStatus);
			assert (0);
		}
		break;
		
	default:
		assert (0);
		break;
	}
}

void CDWHCIFrameSchedulerPeriodic::WaitForFrame (void)
{
	CDWHCIRegister FrameNumber (DWHCI_HOST_FRM_NUM);

	if (m_nNextFrame == FRAME_UNSET)
	{
		m_nNextFrame = (DWHCI_HOST_FRM_NUM_NUMBER (FrameNumber.Read ()) + 1) & 7;
		if (m_nNextFrame == 6)
		{
			m_nNextFrame++;
		}
	}

	while ((DWHCI_HOST_FRM_NUM_NUMBER (FrameNumber.Read ()) & 7) != m_nNextFrame)
	{
		// do nothing
	}
}

boolean CDWHCIFrameSchedulerPeriodic::IsOddFrame (void) const
{
	return m_nNextFrame & 1 ? TRUE : FALSE;
}
