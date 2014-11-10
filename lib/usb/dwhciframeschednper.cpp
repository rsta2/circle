//
// dwhciframeschednper.cpp
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
#include <circle/usb/dwhciframeschednper.h>
#include <circle/usb/dwhci.h>
#include <circle/logger.h>
#include <assert.h>

#define uFRAME			125		// micro seconds

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

CDWHCIFrameSchedulerNonPeriodic::CDWHCIFrameSchedulerNonPeriodic (void)
:	m_pTimer (CTimer::Get ()),
	m_nState (StateUnknown)
{
	assert (m_pTimer != 0);
}

CDWHCIFrameSchedulerNonPeriodic::~CDWHCIFrameSchedulerNonPeriodic (void)
{
	m_nState = StateUnknown;
}

void CDWHCIFrameSchedulerNonPeriodic::StartSplit (void)
{
	m_nState = StateStartSplit;
}

boolean CDWHCIFrameSchedulerNonPeriodic::CompleteSplit (void)
{
	boolean bResult = FALSE;
	
	switch (m_nState)
	{
	case StateStartSplitComplete:
		m_nState = StateCompleteSplit;
		m_nTries = 3;
		bResult = TRUE;
		break;

	case StateCompleteSplit:
	case StateCompleteRetry:
		m_pTimer->usDelay (5 * uFRAME);
		bResult = TRUE;
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

void CDWHCIFrameSchedulerNonPeriodic::TransactionComplete (u32 nStatus)
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
			}
			else
			{
				m_nState = StateCompleteRetry;
			}
		}
		else if (nStatus & DWHCI_HOST_CHAN_INT_NAK)
		{
			if (m_nTries-- == 0)
			{
				m_pTimer->usDelay (5 * uFRAME);
				m_nState = StateCompleteSplitFailed;
			}
			else
			{
				m_nState = StateCompleteRetry;
			}
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

void CDWHCIFrameSchedulerNonPeriodic::WaitForFrame (void)
{
}

boolean CDWHCIFrameSchedulerNonPeriodic::IsOddFrame (void) const
{
	return FALSE;
}
