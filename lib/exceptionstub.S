/*
 * exceptionstub.S
 *
 * Circle - A C++ bare metal environment for Raspberry Pi
 * Copyright (C) 2014-2015  R. Stange <rsta2@o2online.de>
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

	.macro	stub name, exception, pc_offset

	.globl	\name
\name:
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
	cps	#0x1F				/* our abort handler runs in system mode */
	mov	sp, r1				/* set sp_sys to stack top of abort stack */
	stmfd	sp!, {r2, r3}			/* store lr_irq and sp_irq onto stack */
	mov	r1, sp				/* r1: pointer to register frame */
	mov	r0, #\exception			/* r0: exception identifier */
	b	ExceptionHandler		/* jump to ExceptionHandler (never returns) */

	.endm

	.text

/*
 * Abort stubs
 */
	stub	UndefinedInstructionStub,	EXCEPTION_UNDEFINED_INSTRUCTION,	4
	stub	PrefetchAbortStub,		EXCEPTION_PREFETCH_ABORT,		4
	stub	DataAbortStub,			EXCEPTION_DATA_ABORT,			8

/*
 * IRQ stub
 */
	.globl	IRQStub
IRQStub:
	sub	lr, lr, #4			/* lr: return address */
	stmfd	sp!, {r0-r12, lr}		/* save r0-r12 and return address */
	bl	InterruptHandler
	ldmfd	sp!, {r0-r12, pc}^		/* restore registers and return */

/* End */
