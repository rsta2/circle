//
// taskswitch.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_sched_taskswitch_h
#define _circle_sched_taskswitch_h

#include <circle/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct TTaskRegisters
{
	u32	r0;
	u32	r1;
	u32	r2;
	u32	r3;
	u32	r4;
	u32	r5;
	u32	r6;
	u32	r7;
	u32	r8;
	u32	r9;
	u32	r10;
	u32	r11;
	u32	r12;
	u32	sp;
	u32	lr;
};

void TaskSwitch (TTaskRegisters *pOldRegs, TTaskRegisters *pNewRegs);

#ifdef __cplusplus
}
#endif

#endif
