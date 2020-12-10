/*
 * util_fast.S
 *
 * Portions are taken from the "copies and fills" library by Simon Hall
 * which is licensed under the GNU Lesser General Public License version 2.1
 *
 * Circle - A C++ bare metal environment for Raspberry Pi
 * Copyright (C) 2016-2020  R. Stange <rsta2@o2online.de>
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

	.text

#if AARCH == 32

	.globl	memset
memset:
	cmp	r2, #15
	and	ip, r0, #3
	movls	r3, #0
	movhi	r3, #1
	cmp	ip, #0
	movne	r3, #0
	cmp	r3, #0
	bne	2f
	cmp	r2, #0
	mov	r3, r0
	bxeq	lr

	add	r2, r0, r2
1:	strb	r1, [r3], #1
	cmp	r3, r2
	bne	1b
	bx	lr

2:	sub	r3, r2, #16
	push	{r4, r5, lr}
	bic	r5, r3, #15
	orr	lr, r1, r1, lsl #8
	add	r4, r0, #32
	orr	lr, lr, lr, lsl #16
	add	ip, r0, #16
	add	r4, r4, r5
	lsr	r3, r3, #4
3:	str	lr, [ip, #-16]
	str	lr, [ip, #-12]
	str	lr, [ip, #-8]
	str	lr, [ip, #-4]
	add	ip, ip, #16
	cmp	ip, r4
	bne	3b
	and	r2, r2, #15
	add	r3, r3, #1
	cmp	r2, #0
	add	r3, r0, r3, lsl #4
	popeq	{r4, r5, pc}

	add	r2, r3, r2
4:	strb	r1, [r3], #1
	cmp	r3, r2
	bne	4b
	pop	{r4, r5, pc}

	.globl	memcpy
memcpy:
	push	{r0}

	cmp	r2, #127
	bls	2f
	tst	r1, #3
	bne	2f
	tst	r0, #3
	bne	2f

	push	{r4-r10}
1:	ldmia	r1!, {r3-r10}
	sub	r2, #8*4
	stmia	r0!, {r3-r10}
	pld	[r1, #8*4*2]
	cmp	r2, #8*4-1
	bhi	1b
	pop	{r4-r10}

2:	cmp	r2, #0
	beq	4f

3:	ldrb	r3, [r1], #1
	subs	r2, #1
	strb	r3, [r0], #1
	bne	3b

4:	pop	{r0}
	bx	lr

#else

	.globl	memset
memset:
	tst	x0, #3
	mov	x3, x0
	ccmp	x2, #15, #0, eq
	b.hi	4f
1:	add	x4, x3, x2
	cbz	x2, 3f
2:	strb	w1, [x3], #1
	cmp	x3, x4
	b.ne	2b
3:	ret

4:	sub	x5, x2, #16
	orr	w4, w1, w1, lsl #8
	and	x5, x5, #-16
	add	x5, x5, #16
	orr	w4, w4, w4, lsl #16
	add	x5, x0, x5
5:	stp	w4, w4, [x3]
	add	x3, x3, #16
	stur	w4, [x3, #-8]
	cmp	x5, x3
	stur	w4, [x3, #-4]
	b.ne	5b
	and	x2, x2, #15
	b 	1b

	.globl	memcpy
memcpy:
	mov	x8, x0

	cmp	x2, #127
	b.ls	2f
	tst	x1, #7
	b.ne	2f
	tst	x1, #7
	b.ne	2f

	mov	x3, #64
1:	ldp	x4, x5, [x1], #16
	ldp	x6, x7, [x1], #16
	sub	x2, x2, #32
	stp	x4, x5, [x0], #16
	stp	x6, x7, [x0], #16
	prfm	pldl1strm, [x1, x3]
	cmp	x2, #32-1
	b.hi	1b

2:	cmp	x2, #0
	b.eq	4f

3:	ldrb	w3, [x1], #1
	subs	x2, x2, #1
	strb	w3, [x0], #1
	b.ne	3b

4:	mov	x0, x8
	ret

#endif

/* End */
