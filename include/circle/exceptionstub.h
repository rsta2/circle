//
// exceptionstub.h
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
#ifndef _circle_exceptionstub_h
#define _circle_exceptionstub_h

#include <circle/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARM_OPCODE_BRANCH(distance)	(0xEA000000 | (distance))
#define ARM_DISTANCE(from, to)		((u32 *) &(to) - (u32 *) &(from) - 2)

struct TExceptionTable
{
	//u32 Reset;
	u32 UndefinedInstruction;
	u32 SupervisorCall;
	u32 PrefetchAbort;
	u32 DataAbort;
	u32 Unused;
	u32 IRQ;
	u32 FIQ;
};

#define ARM_EXCEPTION_TABLE_BASE	0x00000004

struct TAbortFrame
{
	u32	sp_irq;
	u32	lr_irq;
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
	u32	spsr;
	u32	pc;
};

void UndefinedInstructionStub (void);
void PrefetchAbortStub (void);
void DataAbortStub (void);
void IRQStub (void);

void ExceptionHandler (u32 nException, TAbortFrame *pFrame);
void InterruptHandler (void);

#ifdef __cplusplus
}
#endif

#endif
