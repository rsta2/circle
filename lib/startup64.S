/*
 * startup64.S
 *
 * Circle - A C++ bare metal environment for Raspberry Pi
 * Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
 *
 * This file contains (sligthly modified) code taken from U-Boot:
 *	armv8_switch_to_el1_m macro
 *	defined in arch/arm/include/asm/macro.h
 *	Copyright (C) 2009 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *	Licensed under GPL-2.0+
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

	.macro armv8_switch_to_el1_m, xreg1, xreg2

	/* Initialize Generic Timers */
	mrs	\xreg1, cnthctl_el2
	orr	\xreg1, \xreg1, #0x3	/* Enable EL1 access to timers */
	msr	cnthctl_el2, \xreg1
	msr	cntvoff_el2, xzr

	/* Initilize MPID/MPIDR registers */
	mrs	\xreg1, midr_el1
	mrs	\xreg2, mpidr_el1
	msr	vpidr_el2, \xreg1
	msr	vmpidr_el2, \xreg2

	/* Disable coprocessor traps */
	mov	\xreg1, #0x33ff
	msr	cptr_el2, \xreg1	/* Disable coprocessor traps to EL2 */
	msr	hstr_el2, xzr		/* Disable coprocessor traps to EL2 */
	mov	\xreg1, #3 << 20
	msr	cpacr_el1, \xreg1	/* Enable FP/SIMD at EL1 */

	/* Initialize HCR_EL2 */
	mov	\xreg1, #(1 << 31)		/* 64bit EL1 */
	msr	hcr_el2, \xreg1

	/* SCTLR_EL1 initialization
	 *
	 * setting RES1 bits (29,28,23,22,20,11) to 1
	 * and RES0 bits (31,30,27,21,17,13,10,6) +
	 * UCI,EE,EOE,WXN,nTWE,nTWI,UCT,DZE,I,UMA,SED,ITD,
	 * CP15BEN,SA0,SA,C,A,M to 0
	 */
	mov	\xreg1, #0x0800
	movk	\xreg1, #0x30d0, lsl #16
	msr	sctlr_el1, \xreg1

	/* Return to the EL1_SP1 mode from EL2 */
	mov	\xreg1, #0x3c4
	msr	spsr_el2, \xreg1	/* EL1_SP0 | D | A | I | F */
	adr	\xreg1, 1f
	msr	elr_el2, \xreg1
	eret
1:

	.endm

	.section .init

	.globl	_start
_start:					/* normally entered from armstub8 in EL2 after boot */

	mrs	x0, CurrentEL		/* check if already in EL1t mode? */
	cmp	x0, #4
	beq	1f

	ldr	x0, =MEM_EXCEPTION_STACK /* IRQ, FIQ and exception handler run in EL1h */
	msr	sp_el1, x0		/* init their stack */

	ldr	x0, =VectorTable	/* init exception vector table for EL2 */
	msr	vbar_el2, x0

	armv8_switch_to_el1_m x0, x1

1:	ldr	x0, =MEM_KERNEL_STACK	/* main thread runs in EL1t and uses sp_el0 */
	mov	sp, x0			/* init its stack */

	ldr	x0, =VectorTable	/* init exception vector table */
	msr	vbar_el1, x0

	b	sysinit

#ifdef ARM_ALLOW_MULTI_CORE

	.globl	_start_secondary
_start_secondary:			/* normally entered from armstub8 in EL2 after boot */

	mrs	x2, mpidr_el1		/* read affinity */
	and	x2, x2, #CORES-1	/* get CPU ID */

	mrs	x0, CurrentEL		/* check if already in EL1t mode? */
	cmp	x0, #4
	beq	1f

	mov	x1, #EXCEPTION_STACK_SIZE /* calculate exception stack offset for core */
	mul	x1, x1, x2
	ldr	x0, =MEM_EXCEPTION_STACK /* IRQ, FIQ and exception handler run in EL1h */
	add	x0, x0, x1
	msr	sp_el1, x0		/* init their stack */

	armv8_switch_to_el1_m x0, x1

1:	mov	x1, #KERNEL_STACK_SIZE	/* calculate kernel stack offset for core */
	mul	x1, x1, x2
	ldr	x0, =MEM_KERNEL_STACK	/* main thread runs in EL1t and uses sp_el0 */
	add	x0, x0, x1
	mov	sp, x0			/* init its stack */

	ldr	x0, =VectorTable	/* init exception vector table */
	msr	vbar_el1, x0

	b	sysinit_secondary

#endif

/* End */
