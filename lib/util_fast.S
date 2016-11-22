/*
 * util_fast.S
 *
 * Portions are taken from the "copies and fills" library by Simon Hall
 * which is licensed under the GNU Lesser General Public License version 2.1
 */

	.text

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

/* End */
