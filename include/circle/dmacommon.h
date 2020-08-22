//
/// \file dmacommon.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_dmacommon_h
#define _circle_dmacommon_h

#include <circle/types.h>

enum TDREQ
{
	DREQSourceNone	 = 0,
#if RASPPI >= 4
	DREQSourcePWM1	 = 1,
#endif
	DREQSourcePCMTX	 = 2,
	DREQSourcePCMRX	 = 3,
	DREQSourcePWM	 = 5,
	DREQSourceSPITX	 = 6,
	DREQSourceSPIRX	 = 7,
	DREQSourceEMMC	 = 11,
	DREQSourceUARTTX = 12,
	DREQSourceUARTRX = 14
};

typedef void TDMACompletionRoutine (unsigned nChannel, boolean bStatus, void *pParam);

#endif
