//
// taskswitch.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2018  R. Stange <rsta2@o2online.de>
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

#include <circle/macros.h>
#include <circle/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if AARCH == 32

struct TTaskRegisters
{
	u32	r0;
	u32	fpexc;
	u32	fpscr;
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
	u64	d[16];
}
PACKED;

#else

// see: "Procedure Call Standard for the ARM 64-bit Architecture (AArch64)"
struct TTaskRegisters
{
	u64	x0;	// parameter for CTask::TaskEntry()

	u64	x16;	// unknown role (on this platform)
	u64	x17;
	u64	x18;

	u64	x19;	// callee-saved registers
	u64	x20;
	u64	x21;
	u64	x22;
	u64	x23;
	u64	x24;
	u64	x25;
	u64	x26;
	u64	x27;
	u64	x28;

	u64	x29;	// frame pointer
	u64	x30;	// link register
	u64	sp;	// stack pointer

	u64	d8;	// SIMD and floating-point registers
	u64	d9;
	u64	d10;
	u64	d11;
	u64	d12;
	u64	d13;
	u64	d14;
	u64	d15;

	u64	fpcr;	// floating-point control register
	u64	fpsr;	// floating-point status register
}
PACKED;

#endif

void TaskSwitch (TTaskRegisters *pOldRegs, TTaskRegisters *pNewRegs);

#ifdef __cplusplus
}
#endif

#endif
