//
// dwhciframeschedper.cpp
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
#include <circle/usb/dwhciframeschedper.h>
#include <circle/usb/dwhciregister.h>
#include <circle/usb/dwhci.h>
#include <circle/logger.h>
#include <assert.h>

#define uFRAME			125		// micro seconds

#define FRAME_UNSET		(DWHCI_MAX_FRAME_NUMBER+1)

enum TFrameSchedulerState
{
	StateStartSplit,
	StateStartSplitComplete,
	StateCompleteSplit,
	StateCompleteRetry,
	StateCompleteSplitComplete,
	StateCompleteSplitFailed,
#ifdef USE_USB_SOF_INTR
	StatePeriodicDelay,
#endif
	StateUnknown
};

CDWHCIFrameSchedulerPeriodic::CDWHCIFrameSchedulerPeriodic (void)
:	m_pTimer (CTimer::Get ()),
	m_nState (StateUnknown),
#ifdef USE_USB_SOF_INTR
	m_usFrameOffset (FRAME_UNSET),
#endif
	m_usNextFrame (FRAME_UNSET)
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
#ifdef USE_USB_SOF_INTR
	if (   m_nState != StateCompleteSplitFailed
	    && m_nState != StatePeriodicDelay)
	{
		m_usFrameOffset = 1;
	}
#else
	m_usNextFrame = FRAME_UNSET;
#endif
	m_nState = StateStartSplit;
}

boolean CDWHCIFrameSchedulerPeriodic::CompleteSplit (void)
{
	boolean bResult = FALSE;
	
	switch (m_nState)
	{
	case StateStartSplitComplete:
		m_nState = StateCompleteSplit;
		assert (m_usNextFrame != FRAME_UNSET);
#ifndef USE_USB_SOF_INTR
		m_nTries = m_usNextFrame != 5 ? 3 : 2;
		m_usNextFrame = (m_usNextFrame  + 2) & 7;
#else
		m_nTries = (m_usNextFrame & 7) != 5 ? 2 : 1;
#ifndef USE_USB_FIQ
		m_usFrameOffset = 2;
#else
		m_usFrameOffset = 1;	// with FIQ we can act on the next frame already
#endif
#endif
		bResult = TRUE;
		break;

	case StateCompleteRetry:
		bResult = TRUE;
#ifndef USE_USB_SOF_INTR
		assert (m_usNextFrame != FRAME_UNSET);
		m_usNextFrame = (m_usNextFrame + 1) & 7;
#else
		m_usFrameOffset = 1;
#endif
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

#ifndef USE_USB_SOF_INTR
				m_pTimer->usDelay (8 * uFRAME);
#else
				m_usFrameOffset = 3;
#endif
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

void CDWHCIFrameSchedulerPeriodic::WaitForFrame (void)
{
	CDWHCIRegister FrameNumber (DWHCI_HOST_FRM_NUM);

	if (m_usNextFrame == FRAME_UNSET)
	{
		m_usNextFrame = (DWHCI_HOST_FRM_NUM_NUMBER (FrameNumber.Read ()) + 1) & 7;
		if (m_usNextFrame == 6)
		{
			m_usNextFrame++;
		}
	}

	while ((DWHCI_HOST_FRM_NUM_NUMBER (FrameNumber.Read ()) & 7) != m_usNextFrame)
	{
		// do nothing
	}
}

#else

u16 CDWHCIFrameSchedulerPeriodic::GetFrameNumber (void)
{
	CDWHCIRegister FrameNumber (DWHCI_HOST_FRM_NUM);
	u16 usFrameNumber = DWHCI_HOST_FRM_NUM_NUMBER (FrameNumber.Read ());

	assert (m_usFrameOffset != FRAME_UNSET);
	m_usNextFrame = (usFrameNumber+m_usFrameOffset) & DWHCI_MAX_FRAME_NUMBER;

	if (   m_nState == StateStartSplit
	    && (m_usNextFrame & 7) == 6)
	{
		m_usNextFrame++;
	}

	return m_usNextFrame;
}

void CDWHCIFrameSchedulerPeriodic::PeriodicDelay (u16 usFrameOffset)
{
	m_nState = StatePeriodicDelay;

	m_usFrameOffset = usFrameOffset;
	m_usNextFrame = FRAME_UNSET;
}

#endif

boolean CDWHCIFrameSchedulerPeriodic::IsOddFrame (void) const
{
	CDWHCIRegister FrameNumber (DWHCI_HOST_FRM_NUM);
	return !!(FrameNumber.Read () & 1);
}

IMPLEMENT_CLASS_ALLOCATOR (CDWHCIFrameSchedulerPeriodic)
