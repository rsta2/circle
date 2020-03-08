/*
 * exceptionstub.S
 *
 * Circle - A C++ bare metal environment for Raspberry Pi
 * Copyright (C) 2014-2020 R. Stange <rsta2@o2online.de>
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

	.macro	stub name, exception, pc_offset

	.globl	\name
\name:
	sub	sp, sp, #4			/* correct stack (number of pushs must be even) */
	sub	lr, lr, #\pc_offset		/* lr: correct PC of aborted program */
	stmfd	sp!, {lr}			/* store PC onto stack */
	mrs	lr, spsr			/* lr can be overwritten now */
	stmfd	sp!, {lr}			/* store saved PSR onto stack */
	stmfd	sp, {r0-r14}^			/* store user registers r0-r14 (unbanked) */
	sub	sp, sp, #4*15			/* correct stack (not done by previous instruction */
	mov	r1, sp				/* save sp_abt or sp_und */
	cps	#0x12				/* set IRQ mode to access sp_irq and lr_irq */
	mov	r2, sp
	mov	r3, lr
	cps	#0x11				/* set FIQ mode to access sp_fiq and lr_fiq */
	mov	r4, sp
	mov	r5, lr
	cps	#0x1F				/* our abort handler runs in system mode */
	mov	sp, r1				/* set sp_sys to stack top of abort stack */
	stmfd	sp!, {r2-r5}			/* store lr_fiq, sp_fiq, lr_irq, sp_irq onto stack */
	mov	r1, sp				/* r1: pointer to register frame */
	mov	r0, #\exception			/* r0: exception identifier */
	b	ExceptionHandler		/* jump to ExceptionHandler (never returns) */

	.endm

	.text

/*
 * Abort stubs
 */
	stub	UndefinedInstructionInternal,	EXCEPTION_UNDEFINED_INSTRUCTION,	4
	stub	PrefetchAbortStub,		EXCEPTION_PREFETCH_ABORT,		4
	stub	DataAbortStub,			EXCEPTION_DATA_ABORT,			8

/*
 * Undefined Instruction stub
 *
 * TODO: VFP support code
 */
	.globl UndefinedInstructionStub
UndefinedInstructionStub:
	stmfd	sp!, {r0, lr}			/* save r0 and return address */
	fmrx	r0, fpexc			/* check for floating point exception */
#define VFP_FPEXC_EX	(1 << 31)
	tst	r0, #VFP_FPEXC_EX		/* if EX bit is clear in FPEXC */
	beq	UndefinedInstructionInternal	/* then jump to abort stub */
	bic	r0, r0, #VFP_FPEXC_EX		/* else clear EX bit */
	fmxr	fpexc, r0
	ldmfd	sp!, {r0, pc}^			/* restore registers and return */

/*
 * IRQ stub
 */
	.globl	IRQStub
IRQStub:
	sub	lr, lr, #4			/* lr: return address */
	stmfd	sp!, {r0-r3, r12, lr}		/* save r0-r3, r12 and return address */
#ifdef SAVE_VFP_REGS_ON_IRQ
	sub	sp, sp, #4			/* correct stack (number of pushs must be even) */
	vmrs	r0, fpscr			/* save VFP registers */
	stmfd	sp!, {r0}
	vstmdb	sp!, {d0-d15}
#if RASPPI >= 2 && defined (__FAST_MATH__)
	vstmdb	sp!, {d16-d31}
#endif
#endif
	ldr	r0, =IRQReturnAddress		/* store return address for profiling */
	str	lr, [r0]
	bl	InterruptHandler
#ifdef SAVE_VFP_REGS_ON_IRQ
#if RASPPI >= 2 && defined (__FAST_MATH__)
	vldmia	sp!, {d16-d31}
#endif
	vldmia	sp!, {d0-d15}			/* restore VFP registers */
	ldmfd	sp!, {r0}
	vmsr	fpscr, r0
	add	sp, sp, #4			/* correct stack */
#endif
	ldmfd	sp!, {r0-r3, r12, pc}^		/* restore registers and return */

/*
 * FIQ stub
 */
	.globl	FIQStub
FIQStub:
	sub	lr, lr, #4			/* lr: return address */
	stmfd	sp!, {r0-r3, r12, lr}		/* save r0-r3, r12 and return address */
#ifdef SAVE_VFP_REGS_ON_FIQ
	sub	sp, sp, #4			/* correct stack (number of pushs must be even) */
	vmrs	r0, fpscr			/* save VFP registers */
	stmfd	sp!, {r0}
	vstmdb	sp!, {d0-d15}
#if RASPPI >= 2 && defined (__FAST_MATH__)
	vstmdb	sp!, {d16-d31}
#endif
#endif
#if RASPPI == 1
	mov	r3, #0
	mcr	p15, 0, r3, c7, c10, 5		/* PeripheralExit() */
#endif
	ldr	r2, =FIQData
	ldr	r1, [r2]			/* get FIQData.pHandler */
	cmp	r1, #0				/* is handler set? */
	beq	1f
	ldr	r0, [r2, #4]			/* get FIQData.pParam */
	blx	r1				/* call handler (saves r4-r7) */
#if RASPPI == 1
	mov	r3, #0
	mcr	p15, 0, r3, c7, c10, 4		/* PeripheralEntry() */
#endif
#ifdef SAVE_VFP_REGS_ON_FIQ
#if RASPPI >= 2 && defined (__FAST_MATH__)
	vldmia	sp!, {d16-d31}
#endif
	vldmia	sp!, {d0-d15}			/* restore VFP registers */
	ldmfd	sp!, {r0}
	vmsr	fpscr, r0
	add	sp, sp, #4			/* correct stack */
#endif
	ldmfd	sp!, {r0-r3, r12, pc}^		/* restore registers and return */

1:
#if RASPPI == 1
	mov	r3, #0
	mcr	p15, 0, r3, c7, c10, 4		/* PeripheralEntry() */
#endif
	ldr	r1, =ARM_IC_FIQ_CONTROL		/* disable fiq (if handler is not set) */
	mov	r0, #0
	str	r0, [r1]
#if RASPPI == 1
	mov	r3, #0
	mcr	p15, 0, r3, c7, c10, 5		/* PeripheralExit() */
	mcr	p15, 0, r3, c7, c10, 4		/* PeripheralEntry() */
#endif
#ifdef SAVE_VFP_REGS_ON_FIQ
#if RASPPI >= 2 && defined (__FAST_MATH__)
	vldmia	sp!, {d16-d31}
#endif
	vldmia	sp!, {d0-d15}			/* restore VFP registers */
	ldmfd	sp!, {r0}
	vmsr	fpscr, r0
	add	sp, sp, #4			/* correct stack */
#endif
	ldmfd	sp!, {r0-r3, r12, pc}^		/* restore registers and return */

#if RASPPI >= 4

/*
 * SMC stub
 */
	.globl	SMCStub
SMCStub:
	ldr	sp, =SMCStack
	push	{lr}
	bl	SecureMonitorHandler
	pop	{lr}
	movs	pc, lr

#endif

	.data

	.align	2

	.globl	FIQData
FIQData:					/* matches TFIQData: */
	.word	0				/* pHandler */
	.word	0				/* pParam */
	.word	0				/* nFIQNumber */

	.globl	IRQReturnAddress
IRQReturnAddress:
	.word	0

#if RASPPI >= 4

	.bss

	.align	2

	.space	128
SMCStack:

#endif

/* End */
