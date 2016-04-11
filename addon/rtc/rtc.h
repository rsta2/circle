//
// rtc.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
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
#ifndef _rtc_rtc_h
#define _rtc_rtc_h

#include <circle/time.h>
#include <circle/types.h>

class CRealTimeClock
{
public:
	virtual ~CRealTimeClock (void) {}

	virtual boolean Get (CTime *pTime) = 0;

	virtual boolean Set (const CTime &Time) = 0;
};

#endif
