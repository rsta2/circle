//
// firmwarertc.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2024  R. Stange <rsta2@o2online.de>
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
#ifndef _rtc_firmwarertc_h
#define _rtc_firmwarertc_h

#include <rtc/rtc.h>
#include <circle/time.h>
#include <circle/types.h>

class CFirmwareRTC : public CRealTimeClock
{
public:
	boolean Initialize (void);

	boolean Get (CTime *pUTCTime) override;
	boolean Set (const CTime &UTCTime) override;

#ifndef NDEBUG
	void DumpStatus (void);
#endif
};

#endif
