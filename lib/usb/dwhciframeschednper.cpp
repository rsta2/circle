//
// dwhciframeschednper.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2022  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/dwhciregister.h>
#include <circle/usb/dwhci.h>
#include <circle/koptions.h>
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
#ifdef USE_USB_SOF_INTR
	, m_usFrameOffset (8)
#endif
{
	assert (m_pTimer != 0);
}

CDWHCIFrameSchedulerNonPeriodic::~CDWHCIFrameSchedulerNonPeriodic (void)
{
	m_nState = StateUnknown;
}

void CDWHCIFrameSchedulerNonPeriodic::StartSplit (void)
{
#ifdef USE_USB_SOF_INTR
	if (m_nState != StateCompleteSplitFailed)
	{
		m_usFrameOffset = 1;
	}
#endif
	m_nState = StateStartSplit;
}

boolean CDWHCIFrameSchedulerNonPeriodic::CompleteSplit (void)
{
	boolean bResult = FALSE;
#ifdef USE_USB_SOF_INTR
	m_usFrameOffset = 2;
#endif

	switch (m_nState)
	{
	case StateStartSplitComplete:
		m_nState = StateCompleteSplit;
		m_nTries = 3;
		bResult = TRUE;
		break;

	case StateCompleteSplit:
	case StateCompleteRetry:
#ifndef USE_USB_SOF_INTR
		m_pTimer->usDelay (5 * uFRAME);
#else
		m_usFrameOffset = 1;
#endif
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
#ifdef USE_USB_SOF_INTR
				m_usFrameOffset = 1;
#endif
				m_nState = StateCompleteSplitFailed;
			}
			else
			{
				m_nState = StateCompleteRetry;
			}
		}
		else if (nStatus & DWHCI_HOST_CHAN_INT_NAK)
		{
#ifndef USE_USB_SOF_INTR
			m_pTimer->usDelay (5 * uFRAME);
#else
			m_usFrameOffset = 5;
#endif
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

#ifndef USE_USB_SOF_INTR

void CDWHCIFrameSchedulerNonPeriodic::WaitForFrame (void)
{
}

#else

u16 CDWHCIFrameSchedulerNonPeriodic::GetFrameNumber (void)
{
	CDWHCIRegister FrameNumber (DWHCI_HOST_FRM_NUM);
	u16 usFrameNumber = DWHCI_HOST_FRM_NUM_NUMBER (FrameNumber.Read ());

	if (CKernelOptions::Get ()->GetUSBBoost ())
	{
		return usFrameNumber;
	}

	assert (m_usFrameOffset < 8);
	return (usFrameNumber+m_usFrameOffset) & DWHCI_MAX_FRAME_NUMBER;
}

void CDWHCIFrameSchedulerNonPeriodic::PeriodicDelay (u16 usFrameOffset)
{
	assert (0);
}

#endif

boolean CDWHCIFrameSchedulerNonPeriodic::IsOddFrame (void) const
{
	return FALSE;
}

IMPLEMENT_CLASS_ALLOCATOR (CDWHCIFrameSchedulerNonPeriodic)
