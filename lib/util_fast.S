/*
 * util_fast.S
 *
 * Portions are taken from the "copies and fills" library by Simon Hall
 * which is licensed under the GNU Lesser General Public License version 2.1
 */

	.text

	.globl	memcpyblk
memcpyblk:
	push	{r0, r3-r6}
1:	ldmia	r1!, {r3-r6}
	subs	r2, #4*4
	stmia	r0!, {r3-r6}
	pld	[r1, #4*4*4]
	bne	1b
	pop	{r0, r3-r6}
	bx	lr

/* End */
