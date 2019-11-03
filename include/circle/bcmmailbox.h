//
// bcmmailbox.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2019  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_bcmmailbox_h
#define _circle_bcmmailbox_h

#include <circle/bcm2835.h>
#include <circle/spinlock.h>
#include <circle/types.h>

class CBcmMailBox
{
public:
	CBcmMailBox (unsigned nChannel,
		     boolean bEarlyUse = FALSE);	// do not use spinlock early
	~CBcmMailBox (void);

	u32 WriteRead (unsigned nData);

private:
	void Flush (void);

	u32 Read (void);
	void Write (u32 nData);

private:
	unsigned m_nChannel;
	boolean m_bEarlyUse;

	static CSpinLock s_SpinLock;
};

#endif
