/*
 * setjmp.S
 *
 * Circle - A C++ bare metal environment for Raspberry Pi
 * Copyright (C) 2020  R. Stange <rsta2@o2online.de>
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

	.globl	setjmp
setjmp:
	stm	r0, {r4-r14}
	mov	r0, #0
	mov	pc, lr

	.globl	longjmp
longjmp:
	ldm	r0, {r4-r14}
	mov	r0, r1
	cmp	r0, #0
	moveq	r0, #1
	mov	pc, lr

#else

	.globl	setjmp
setjmp:
	mov	x2, sp
	stp	x19, x20, [x0]
	stp	x21, x22, [x0, #16]
	stp	x23, x24, [x0, #32]
	stp	x25, x26, [x0, #48]
	stp	x27, x28, [x0, #64]
	stp	x29, x30, [x0, #80]
	str	x2, [x0, #96]
	mov	w0, #0
	ret

	.globl	longjmp
longjmp:
	ldp	x19, x20, [x0]
	ldp	x21, x22, [x0, #16]
	ldp	x23, x24, [x0, #32]
	ldp	x25, x26, [x0, #48]
	ldp	x27, x28, [x0, #64]
	ldp	x29, x30, [x0, #80]
	ldr	x2, [x0, #96]
	mov	sp, x2
	mov	w0, w1
	cmp	w0, #0
	b.ne	1f
	mov	w0, #1
1:	br	x30

#endif

/* End */
