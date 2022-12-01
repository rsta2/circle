//
// dwhciframeschednsplit.cpp
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
#include <circle/usb/dwhciframeschednsplit.h>
#include <circle/usb/dwhciregister.h>
#include <assert.h>

#define FRAME_UNSET	(DWHCI_MAX_FRAME_NUMBER+1)

CDWHCIFrameSchedulerNoSplit::CDWHCIFrameSchedulerNoSplit (boolean bIsPeriodic)
:	m_bIsPeriodic (bIsPeriodic),
	m_usNextFrame (FRAME_UNSET)
{
}

CDWHCIFrameSchedulerNoSplit::~CDWHCIFrameSchedulerNoSplit (void)
{
}

void CDWHCIFrameSchedulerNoSplit::StartSplit (void)
{
	assert (0);
}

boolean CDWHCIFrameSchedulerNoSplit::CompleteSplit (void)
{
	assert (0);
	return FALSE;
}

void CDWHCIFrameSchedulerNoSplit::TransactionComplete (u32 nStatus)
{
	assert (0);
}

#ifndef USE_USB_SOF_INTR

void CDWHCIFrameSchedulerNoSplit::WaitForFrame (void)
{
	CDWHCIRegister FrameNumber (DWHCI_HOST_FRM_NUM);
	m_usNextFrame = (DWHCI_HOST_FRM_NUM_NUMBER (FrameNumber.Read ())+1) & DWHCI_MAX_FRAME_NUMBER;

	if (!m_bIsPeriodic)
	{
		while ((DWHCI_HOST_FRM_NUM_NUMBER (FrameNumber.Read ()) & DWHCI_MAX_FRAME_NUMBER) != m_usNextFrame)
		{
			// do nothing
		}
	}
}

#else

u16 CDWHCIFrameSchedulerNoSplit::GetFrameNumber (void)
{
	CDWHCIRegister FrameNumber (DWHCI_HOST_FRM_NUM);
	m_usNextFrame = (DWHCI_HOST_FRM_NUM_NUMBER (FrameNumber.Read ())+1) & DWHCI_MAX_FRAME_NUMBER;

	return m_usNextFrame;
}

void CDWHCIFrameSchedulerNoSplit::PeriodicDelay (u16 usFrameOffset)
{
	assert (0);

	(void) m_bIsPeriodic;	// to suppress warning from clang compiler
}

#endif

boolean CDWHCIFrameSchedulerNoSplit::IsOddFrame (void) const
{
	CDWHCIRegister FrameNumber (DWHCI_HOST_FRM_NUM);
	return !!(FrameNumber.Read () & 1);
}

IMPLEMENT_CLASS_ALLOCATOR (CDWHCIFrameSchedulerNoSplit)
