//
// sampler.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2018  R. Stange <rsta2@o2online.de>
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
#ifndef _sampler_h
#define _sampler_h

#include <circle/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// returns run time (micro seconds)
u32 Sampler (u32 *pBuffer, size_t nSamples,
	     u32 nTriggerMask, u32 nTriggerLevel,
	     u32 nDelayCount);

#ifdef __cplusplus
}
#endif

#endif
