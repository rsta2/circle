/*
 * startup.S
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

	.text

	.globl	_start
_start:
	cps	#0x12				/* set irq mode */
	mov	sp, #MEM_IRQ_STACK
	cps	#0x17				/* set abort mode */
	mov	sp, #MEM_ABORT_STACK
	cps	#0x1B				/* set "undefined" mode */
	mov	sp, #MEM_ABORT_STACK
	cps	#0x1F				/* set system mode */
	mov	sp, #MEM_KERNEL_STACK
	b	sysinit

#if RASPPI != 1
	
	.globl	_start_secondary
_start_secondary:
#ifdef ARM_ALLOW_MULTI_CORE
	mrc	p15, 0, r0, c0, c0, 5		/* read MPIDR */
	and	r0, r0, #CORES-1		/* get CPU ID */

	mov	r1, #EXCEPTION_STACK_SIZE	/* calculate exception stack offset for core */
	mul	r1, r0, r1
	cps	#0x12				/* set irq mode */
	mov	r2, #MEM_IRQ_STACK
	add	sp, r1, r2
	cps	#0x17				/* set abort mode */
	mov	r2, #MEM_ABORT_STACK
	add	sp, r1, r2
	cps	#0x1B				/* set "undefined" mode */
	add	sp, r1, r2

	mov	r1, #KERNEL_STACK_SIZE		/* calculate kernel stack offset for core */
	mul	r1, r0, r1
	cps	#0x1F				/* set system mode */
	mov	r2, #MEM_KERNEL_STACK
	add	sp, r1, r2
	b	sysinit_secondary
#else
	dsb
1:	wfi
	b	1b
#endif

#endif

/* End */
