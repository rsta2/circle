/*
 * exceptionstub64.S
 *
 * Circle - A C++ bare metal environment for Raspberry Pi
 * Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <circle/sysconfig.h>
#include <circle/exception.h>
#include <circle/bcm2835.h>

	.macro	vector handler

	.align	7

	b	\handler

	.endm
	
	.macro	stub name, exception

	.globl	\name
\name:
	mrs	x0, esr_el1
	mrs	x1, spsr_el1
	mov	x2, x30				/* lr */
	mrs	x3, elr_el1
	mrs	x4, sp_el0
	mov	x5, sp
	mrs	x6, far_el1

	str	x6, [sp, #-16]!
	stp	x4, x5, [sp, #-16]!
	stp	x2, x3, [sp, #-16]!
	stp	x0, x1, [sp, #-16]!

	mov	x0, #\exception
	mov	x1, sp
	b	ExceptionHandler		/* never returns */

	.endm

	.text

	.align	11

	.globl	VectorTable
VectorTable:

	/* from current EL with sp_el0 */
	vector	SynchronousStub
	vector	IRQStub
	vector	FIQStub
	vector	SErrorStub

	/* from current EL with sp_elx, x != 0 */
	vector	SynchronousStub
	vector	IRQStub
	vector	FIQStub
	vector	SErrorStub

	/* from lower EL, target EL minus 1 is AArch64 */
	vector	HVCStub
	vector	UnexpectedStub
	vector	UnexpectedStub
	vector	UnexpectedStub

	/* from lower EL, target EL minus 1 is AArch32 */
	vector	UnexpectedStub
	vector	UnexpectedStub
	vector	UnexpectedStub
	vector	UnexpectedStub

/*
 * Abort stubs
 */
	stub	UnexpectedStub,		EXCEPTION_UNEXPECTED
	stub	SynchronousStub,	EXCEPTION_SYNCHRONOUS
	stub	SErrorStub,		EXCEPTION_SYSTEM_ERROR

/*
 * IRQ stub
 */
	.globl	IRQStub
IRQStub:
	stp	x29, x30, [sp, #-16]!		/* save x29, x30 onto stack */

	mrs	x29, elr_el1			/* save elr_el1, spsr_el1 onto stack */
	mrs	x30, spsr_el1
	stp	x29, x30, [sp, #-16]!
	msr	DAIFClr, #1			/* enable FIQ */

#ifdef SAVE_VFP_REGS_ON_IRQ
	stp	q30, q31, [sp, #-32]!		/* save q0-q31 onto stack */
	stp	q28, q29, [sp, #-32]!
	stp	q26, q27, [sp, #-32]!
	stp	q24, q25, [sp, #-32]!
	stp	q22, q23, [sp, #-32]!
	stp	q20, q21, [sp, #-32]!
	stp	q18, q19, [sp, #-32]!
	stp	q16, q17, [sp, #-32]!
	stp	q14, q15, [sp, #-32]!
	stp	q12, q13, [sp, #-32]!
	stp	q10, q11, [sp, #-32]!
	stp	q8, q9, [sp, #-32]!
	stp	q6, q7, [sp, #-32]!
	stp	q4, q5, [sp, #-32]!
	stp	q2, q3, [sp, #-32]!
	stp	q0, q1, [sp, #-32]!
#endif
	stp	x27, x28, [sp, #-16]!		/* save x0-x28 onto stack */
	stp	x25, x26, [sp, #-16]!
	stp	x23, x24, [sp, #-16]!
	stp	x21, x22, [sp, #-16]!
	stp	x19, x20, [sp, #-16]!
	stp	x17, x18, [sp, #-16]!
	stp	x15, x16, [sp, #-16]!
	stp	x13, x14, [sp, #-16]!
	stp	x11, x12, [sp, #-16]!
	stp	x9, x10, [sp, #-16]!
	stp	x7, x8, [sp, #-16]!
	stp	x5, x6, [sp, #-16]!
	stp	x3, x4, [sp, #-16]!
	stp	x1, x2, [sp, #-16]!
	str	x0, [sp, #-16]!

	ldr	x0, =IRQReturnAddress		/* store return address for profiling */
	str	x29, [x0]

	bl	InterruptHandler

	ldr	x0, [sp], #16			/* restore x0-x28 from stack */
	ldp	x1, x2, [sp], #16
	ldp	x3, x4, [sp], #16
	ldp	x5, x6, [sp], #16
	ldp	x7, x8, [sp], #16
	ldp	x9, x10, [sp], #16
	ldp	x11, x12, [sp], #16
	ldp	x13, x14, [sp], #16
	ldp	x15, x16, [sp], #16
	ldp	x17, x18, [sp], #16
	ldp	x19, x20, [sp], #16
	ldp	x21, x22, [sp], #16
	ldp	x23, x24, [sp], #16
	ldp	x25, x26, [sp], #16
	ldp	x27, x28, [sp], #16
#ifdef SAVE_VFP_REGS_ON_IRQ
	ldp	q0, q1, [sp], #32		/* restore q0-q31 from stack */
	ldp	q2, q3, [sp], #32
	ldp	q4, q5, [sp], #32
	ldp	q6, q7, [sp], #32
	ldp	q8, q9, [sp], #32
	ldp	q10, q11, [sp], #32
	ldp	q12, q13, [sp], #32
	ldp	q14, q15, [sp], #32
	ldp	q16, q17, [sp], #32
	ldp	q18, q19, [sp], #32
	ldp	q20, q21, [sp], #32
	ldp	q22, q23, [sp], #32
	ldp	q24, q25, [sp], #32
	ldp	q26, q27, [sp], #32
	ldp	q28, q29, [sp], #32
	ldp	q30, q31, [sp], #32
#endif

	msr	DAIFSet, #1			/* disable FIQ */
	ldp	x29, x30, [sp], #16		/* restore elr_el1, spsr_el1 from stack */
	msr	elr_el1, x29
	msr	spsr_el1, x30

	ldp	x29, x30, [sp], #16		/* restore x29, x30 from stack */

	eret

/*
 * FIQ stub
 */
	.globl	FIQStub
FIQStub:
#ifdef SAVE_VFP_REGS_ON_FIQ
	stp	q30, q31, [sp, #-32]!
	stp	q28, q29, [sp, #-32]!
	stp	q26, q27, [sp, #-32]!
	stp	q24, q25, [sp, #-32]!
	stp	q22, q23, [sp, #-32]!
	stp	q20, q21, [sp, #-32]!
	stp	q18, q19, [sp, #-32]!
	stp	q16, q17, [sp, #-32]!
	stp	q14, q15, [sp, #-32]!
	stp	q12, q13, [sp, #-32]!
	stp	q10, q11, [sp, #-32]!
	stp	q8, q9, [sp, #-32]!
	stp	q6, q7, [sp, #-32]!
	stp	q4, q5, [sp, #-32]!
	stp	q2, q3, [sp, #-32]!
	stp	q0, q1, [sp, #-32]!
#endif
	stp	x29, x30, [sp, #-16]!
	stp	x27, x28, [sp, #-16]!
	stp	x25, x26, [sp, #-16]!
	stp	x23, x24, [sp, #-16]!
	stp	x21, x22, [sp, #-16]!
	stp	x19, x20, [sp, #-16]!
	stp	x17, x18, [sp, #-16]!
	stp	x15, x16, [sp, #-16]!
	stp	x13, x14, [sp, #-16]!
	stp	x11, x12, [sp, #-16]!
	stp	x9, x10, [sp, #-16]!
	stp	x7, x8, [sp, #-16]!
	stp	x5, x6, [sp, #-16]!
	stp	x3, x4, [sp, #-16]!
	stp	x1, x2, [sp, #-16]!
	str	x0, [sp, #-16]!

	ldr	x2, =FIQData
	ldr	x1, [x2]			/* get FIQData.pHandler */
	cmp	x1, #0				/* is handler set? */
	b.eq	2f
	ldr	x0, [x2, #8]			/* get FIQData.pParam */
	blr	x1				/* call handler */

1:	ldr	x0, [sp], #16
	ldp	x1, x2, [sp], #16
	ldp	x3, x4, [sp], #16
	ldp	x5, x6, [sp], #16
	ldp	x7, x8, [sp], #16
	ldp	x9, x10, [sp], #16
	ldp	x11, x12, [sp], #16
	ldp	x13, x14, [sp], #16
	ldp	x15, x16, [sp], #16
	ldp	x17, x18, [sp], #16
	ldp	x19, x20, [sp], #16
	ldp	x21, x22, [sp], #16
	ldp	x23, x24, [sp], #16
	ldp	x25, x26, [sp], #16
	ldp	x27, x28, [sp], #16
	ldp	x29, x30, [sp], #16
#ifdef SAVE_VFP_REGS_ON_FIQ
	ldp	q0, q1, [sp], #32
	ldp	q2, q3, [sp], #32
	ldp	q4, q5, [sp], #32
	ldp	q6, q7, [sp], #32
	ldp	q8, q9, [sp], #32
	ldp	q10, q11, [sp], #32
	ldp	q12, q13, [sp], #32
	ldp	q14, q15, [sp], #32
	ldp	q16, q17, [sp], #32
	ldp	q18, q19, [sp], #32
	ldp	q20, q21, [sp], #32
	ldp	q22, q23, [sp], #32
	ldp	q24, q25, [sp], #32
	ldp	q26, q27, [sp], #32
	ldp	q28, q29, [sp], #32
	ldp	q30, q31, [sp], #32
#endif

	eret

2:	ldr	x1, =ARM_IC_FIQ_CONTROL		/* disable FIQ (if handler is not set) */
	mov	w0, #0
	str	w0, [x1]
	b	1b

#if RASPPI >= 4

/*
 * SMC stub
 */
	.globl	SMCStub
SMCStub:
	ldr	x2, =SMCStack
	mov	sp, x2
	str	x30, [sp, #-16]!
	bl	SecureMonitorHandler
	ldr	x30, [sp], #16
	eret

#endif

/*
 * HVC stub
 */
HVCStub:					/* return to EL2h mode */
	mrs	x0, spsr_el2
	bic	x0, x0, #0xF
	mov	x1, #9
	orr	x0, x0, x1
	msr	spsr_el2, x0
	eret

	.data

	.align	3

	.globl	FIQData
FIQData:					/* matches TFIQData: */
	.quad	0				/* pHandler */
	.quad	0				/* pParam */
	.word	0				/* nFIQNumber (unused) */

	.align	3

	.globl	IRQReturnAddress
IRQReturnAddress:
	.quad	0

#if RASPPI >= 4

	.bss

	.align	4

	.space	128
SMCStack:

#endif

/* End */
