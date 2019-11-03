/*
 * delayloop.S
 */

	.text

#if AARCH == 32

	.align	3

	.globl	DelayLoop
DelayLoop:
	mov	r1, r1
	mov	r2, r2
	subs	r0, r0, #1
	bhi	DelayLoop
	mov	pc, lr

#else

	.align	6

	.globl	DelayLoop
DelayLoop:
	mov	w1, w1
	mov	w2, w2
	subs	w0, w0, #1
	bne	DelayLoop
	ret

#endif

/* End */
