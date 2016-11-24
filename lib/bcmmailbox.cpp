//
// bcmmailbox.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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
#include <circle/bcmmailbox.h>
#include <circle/memio.h>
#include <circle/synchronize.h>
#include <circle/timer.h>
#include <assert.h>

CBcmMailBox::CBcmMailBox (unsigned nChannel)
:	m_nChannel (nChannel),
	m_SpinLock (FALSE)
{
}

CBcmMailBox::~CBcmMailBox (void)
{
}

unsigned CBcmMailBox::WriteRead (unsigned nData)
{
	PeripheralEntry ();

	m_SpinLock.Acquire ();

	Flush ();

	Write (nData);

	unsigned nResult = Read ();

	m_SpinLock.Release ();

	PeripheralExit ();

	return nResult;
}

void CBcmMailBox::Flush (void)
{
	while (!(read32 (MAILBOX0_STATUS) & MAILBOX_STATUS_EMPTY))
	{
		read32 (MAILBOX0_READ);

		CTimer::SimpleMsDelay (20);
	}
}

unsigned CBcmMailBox::Read (void)
{
	unsigned nResult;
	
	do
	{
		while (read32 (MAILBOX0_STATUS) & MAILBOX_STATUS_EMPTY)
		{
			// do nothing
		}
		
		nResult = read32 (MAILBOX0_READ);
	}
	while ((nResult & 0xF) != m_nChannel);		// channel number is in the lower 4 bits

	return nResult & ~0xF;
}

void CBcmMailBox::Write (unsigned nData)
{
	while (read32 (MAILBOX1_STATUS) & MAILBOX_STATUS_FULL)
	{
		// do nothing
	}

	assert ((nData & 0xF) == 0);
	write32 (MAILBOX1_WRITE, m_nChannel | nData);	// channel number is in the lower 4 bits
}
