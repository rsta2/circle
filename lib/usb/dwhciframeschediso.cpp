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
#include <circle/logger.h>
#include <assert.h>

#define FRAME_UNSET		(DWHCI_MAX_FRAME_NUMBER+1)

enum TFrameSchedulerState
{
	StateStart,
	StateStartSplit,
	StateStartSplitComplete,
	StateStartSplitContinued,
	StateUnknown
};

CDWHCIFrameSchedulerIsochronous::CDWHCIFrameSchedulerIsochronous (void)
:	m_nState (StateStart),
	m_usNextFrame (FRAME_UNSET)
{
}

CDWHCIFrameSchedulerIsochronous::~CDWHCIFrameSchedulerIsochronous (void)
{
	m_nState = StateUnknown;
}

void CDWHCIFrameSchedulerIsochronous::StartSplit (void)
{
#ifndef USE_USB_SOF_INTR
	m_usNextFrame = FRAME_UNSET;
#endif
	m_nState = m_nState == StateStart ? StateStartSplit : StateStartSplitContinued;
}

boolean CDWHCIFrameSchedulerIsochronous::CompleteSplit (void)
{
	assert (0);

	return FALSE;
}

void CDWHCIFrameSchedulerIsochronous::TransactionComplete (u32 nStatus)
{
	assert (nStatus & DWHCI_HOST_CHAN_INT_ACK);

	assert (   m_nState != StateStartSplit
		|| m_nState != StateStartSplitContinued);
	m_nState = StateStartSplitComplete;
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

	assert (   m_nState != StateStartSplit
		|| m_nState != StateStartSplitContinued);
	if (m_nState == StateStartSplit)
	{
		m_usNextFrame = (usFrameNumber + 8) & (DWHCI_MAX_FRAME_NUMBER & ~7);
	}
	else
	{
		m_usNextFrame = (usFrameNumber + 1) & DWHCI_MAX_FRAME_NUMBER;
	}

	return m_usNextFrame;
}

void CDWHCIFrameSchedulerIsochronous::PeriodicDelay (u16 usFrameOffset)
{
	assert (0);
}

#endif

boolean CDWHCIFrameSchedulerIsochronous::IsOddFrame (void) const
{
	return m_usNextFrame & 1 ? TRUE : FALSE;
}

IMPLEMENT_CLASS_ALLOCATOR (CDWHCIFrameSchedulerIsochronous)
