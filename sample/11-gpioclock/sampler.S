/*
 * sampler.S
 *
 * Circle - A C++ bare metal environment for Raspberry Pi
 * Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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
#include <circle/bcm2835.h>

/*
 * Sampler
 *
 * In:	r0	pBuffer
 *	r1	nSamples
 *	r2	nTriggerMask
 *	r3	nTriggerLevel
 *	[sp]	nDelayCount
 *
 * Out:	r0	run time (micro seconds)
 */
	.globl	Sampler
Sampler:
	push	{r4-r7}
	ldr	r4, [sp, #16]

.ifeq RASPPI-1
	mcr	p15, 0, r0, c7, c10, 4		/* data sync barrier */
.endif

	cpsid	i

	ldr	r5, =ARM_GPIO_GPLEV0
1:	ldr	r6, [r5]			/* wait for trigger */
	and	r7, r6, r2
	cmp	r7, r3
	bne	1b

.ifeq RASPPI-1
	mcr	p15, 0, r0, c7, c10, 4		/* data sync barrier */
.endif

	ldr	r2, =ARM_SYSTIMER_CLO
	ldr	r3, [r2]			/* get start time */

	cmp	r4, #0				/* if nDelayCount if 0 */
	beq	fastloop			/* branch to fast loop */

2:	str	r6, [r0], #4			/* sample loop */

	mov	r7, r4				/* delay loop */
3:	subs	r7, r7, #1
	bhi	3b

	ldr	r6, [r5]
	subs	r1, r1, #1
	bhi	2b

4:

.ifeq RASPPI-1
	mcr	p15, 0, r0, c7, c10, 4		/* data sync barrier */
.endif

	ldr	r2, [r2]			/* get end time */

	cpsie	i

.ifeq RASPPI-1
	mcr	p15, 0, r0, c7, c10, 5		/* data mem barrier */
.endif

	sub	r0, r2, r3			/* calculate run time */

	pop	{r4-r7}
	bx	lr

fastloop:
	str	r6, [r0], #4			/* fast sample loop */
	ldr	r6, [r5]
	subs	r1, r1, #1
	bhi	fastloop
	b	4b

/* End */
