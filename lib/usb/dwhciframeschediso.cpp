//
// dwhciframeschediso.cpp
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
#include <circle/usb/dwhciframeschediso.h>
#include <circle/usb/dwhciregister.h>
#include <circle/usb/dwhci.h>
#include <assert.h>

#define FRAME_UNSET		(DWHCI_MAX_FRAME_NUMBER+1)

enum TFrameSchedulerState
{
	StateStart,
	StateStartSplit,
	StateStartSplitComplete,
	StateStartSplitContinued,
	StateCompleteSplit,
	StateCompleteSplitRetry,
	StateCompleteSplitComplete,
	StatePeriodicDelay,
	StateUnknown
};

CDWHCIFrameSchedulerIsochronous::CDWHCIFrameSchedulerIsochronous (boolean bIn)
:	m_bIn (bIn),
	m_nState (StateStart),
	m_usFrameOffset (FRAME_UNSET),
	m_bFrameAlign (FALSE),
	m_usNextFrame (FRAME_UNSET)
{
#ifndef USE_USB_SOF_INTR
	assert (0);
#endif
}

CDWHCIFrameSchedulerIsochronous::~CDWHCIFrameSchedulerIsochronous (void)
{
	m_nState = StateUnknown;
}

void CDWHCIFrameSchedulerIsochronous::StartSplit (void)
{
	switch (m_nState)
	{
	case StateStart:
		m_nState = StateStartSplit;
		m_usFrameOffset = 8;
		m_bFrameAlign = TRUE;
		m_usNextFrame = FRAME_UNSET;
		break;

	case StateStartSplitComplete:
		m_nState = StateStartSplitContinued;
		m_usFrameOffset = 1;
		m_bFrameAlign = FALSE;
		m_usNextFrame = FRAME_UNSET;
		break;

	case StatePeriodicDelay:
		m_nState = StateStartSplit;
		assert (m_usFrameOffset != FRAME_UNSET);
		assert (m_bFrameAlign);
		assert (m_usNextFrame != FRAME_UNSET);
		break;

	default:
		break;
	}
}

boolean CDWHCIFrameSchedulerIsochronous::CompleteSplit (void)
{
	boolean bResult = FALSE;

	switch (m_nState)
	{
	case StateStartSplitComplete:
		m_nState = StateCompleteSplit;
		m_usFrameOffset = 2;
		m_bFrameAlign = FALSE;
		assert (m_usNextFrame != FRAME_UNSET);
		bResult = TRUE;
		m_nRetries = 1;
		break;

	case StateCompleteSplitComplete:
	case StatePeriodicDelay:
		break;

	case StateCompleteSplitRetry:
		m_nState = StateCompleteSplit;
		m_usFrameOffset = 2;
		m_bFrameAlign = FALSE;
		assert (m_usNextFrame != FRAME_UNSET);
		bResult = TRUE;
		break;

	default:
		assert (0);
		break;
	}

	return bResult;
}

void CDWHCIFrameSchedulerIsochronous::TransactionComplete (u32 nStatus)
{
	switch (m_nState)
	{
	case StateStartSplit:
	case StateStartSplitContinued:
		assert (nStatus & DWHCI_HOST_CHAN_INT_ACK);
		m_nState = StateStartSplitComplete;
		break;

	case StateCompleteSplit:
		assert (!(nStatus & DWHCI_HOST_CHAN_INT_NAK));
		if (nStatus & DWHCI_HOST_CHAN_INT_XFER_COMPLETE)
		{
			m_nState = StateCompleteSplitComplete;
		}
		else if (nStatus & DWHCI_HOST_CHAN_INT_NYET)
		{
			if (--m_nRetries)
			{
				m_nState = StateCompleteSplitRetry;
			}
			else
			{
				m_nState = StatePeriodicDelay;
			}
		}
		else
		{
			assert (0);
		}
		break;

	default:
		assert (0);
		break;
	}
}

#ifndef USE_USB_SOF_INTR

void CDWHCIFrameSchedulerIsochronous::WaitForFrame (void)
{
	assert (0);
}

#else

u16 CDWHCIFrameSchedulerIsochronous::GetFrameNumber (void)
{
	CDWHCIRegister FrameNumber (DWHCI_HOST_FRM_NUM);
	u16 usFrameNumber = DWHCI_HOST_FRM_NUM_NUMBER (FrameNumber.Read ());

	assert (m_usFrameOffset != FRAME_UNSET);
	m_usNextFrame = (usFrameNumber + m_usFrameOffset) & DWHCI_MAX_FRAME_NUMBER;

	if (m_bFrameAlign)
	{
		m_usNextFrame = m_usNextFrame & ~7;

		if (m_bIn)
		{
			m_usNextFrame = (m_usNextFrame + 4) & DWHCI_MAX_FRAME_NUMBER;
		}
	}

	return m_usNextFrame;
}

void CDWHCIFrameSchedulerIsochronous::PeriodicDelay (u16 usFrameOffset)
{
	assert (m_nState == StatePeriodicDelay);

	m_usFrameOffset = usFrameOffset;
	m_bFrameAlign = TRUE;
	m_usNextFrame = FRAME_UNSET;
}

#endif

boolean CDWHCIFrameSchedulerIsochronous::IsOddFrame (void) const
{
	CDWHCIRegister FrameNumber (DWHCI_HOST_FRM_NUM);
	return !!(FrameNumber.Read () & 1);
}

IMPLEMENT_CLASS_ALLOCATOR (CDWHCIFrameSchedulerIsochronous)
