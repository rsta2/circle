/*
 * startup.S
 *
 * Circle - A C++ bare metal environment for Raspberry Pi
 * Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
 *
 * This file contains code taken from Linux:
 *	safe_svcmode_maskall macro
 *	defined in arch/arm/include/asm/assembler.h
 *	Copyright (C) 1996-2000 Russell King
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

/*
 * Helper macro to enter SVC mode cleanly and mask interrupts. reg is
 * a scratch register for the macro to overwrite.
 *
 * This macro is intended for forcing the CPU into SVC mode at boot time.
 * you cannot return to the original mode.
 */
	.macro safe_svcmode_maskall reg:req

	mrs	\reg , cpsr
	eor	\reg, \reg, #0x1A		/* test for HYP mode */
	tst	\reg, #0x1F
	bic	\reg , \reg , #0x1F		/* clear mode bits */
	orr	\reg , \reg , #0xC0 | 0x13	/* mask IRQ/FIQ bits and set SVC mode */
	bne	1f				/* branch if not HYP mode */
	orr	\reg, \reg, #0x100		/* mask Abort bit */
	adr	lr, 2f
	msr	spsr_cxsf, \reg
	.word	0xE12EF30E			/* msr ELR_hyp, lr */
	.word	0xE160006E			/* eret */
1:	msr	cpsr_c, \reg
2:

	.endm

	.section .init

	.globl	_start
_start:
#ifndef USE_RPI_STUB_AT
	safe_svcmode_maskall r0

	mov	r0, #0
	mcr	p15, 0, r0, c12, c0, 0		/* reset VBAR (if changed by u-boot) */
#endif
	cps	#0x11				/* set fiq mode */
	ldr	sp, =MEM_FIQ_STACK
	cps	#0x12				/* set irq mode */
	ldr	sp, =MEM_IRQ_STACK
	cps	#0x17				/* set abort mode */
	ldr	sp, =MEM_ABORT_STACK
	cps	#0x1B				/* set "undefined" mode */
	ldr	sp, =MEM_ABORT_STACK
	cps	#0x1F				/* set system mode */
	ldr	sp, =MEM_KERNEL_STACK
	b	sysinit

#if RASPPI != 1
	
	.globl	_start_secondary
_start_secondary:
#ifdef ARM_ALLOW_MULTI_CORE
	safe_svcmode_maskall r0

	mrc	p15, 0, r0, c0, c0, 5		/* read MPIDR */
	and	r0, r0, #CORES-1		/* get CPU ID */

	mov	r1, #EXCEPTION_STACK_SIZE	/* calculate exception stack offset for core */
	mul	r1, r0, r1
	cps	#0x11				/* set fiq mode */
	ldr	r2, =MEM_FIQ_STACK
	add	sp, r1, r2
	cps	#0x12				/* set irq mode */
	ldr	r2, =MEM_IRQ_STACK
	add	sp, r1, r2
	cps	#0x17				/* set abort mode */
	ldr	r2, =MEM_ABORT_STACK
	add	sp, r1, r2
	cps	#0x1B				/* set "undefined" mode */
	add	sp, r1, r2

	mov	r1, #KERNEL_STACK_SIZE		/* calculate kernel stack offset for core */
	mul	r1, r0, r1
	cps	#0x1F				/* set system mode */
	ldr	r2, =MEM_KERNEL_STACK
	add	sp, r1, r2
	b	sysinit_secondary
#else
	dsb
1:	wfi
	b	1b
#endif

#endif

/* End */
