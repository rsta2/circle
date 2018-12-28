/*
 * delayloop.S
 */

	.text

#if AARCH == 32

	.align	3

	.globl	DelayLoop
DelayLoop:
	subs	r0, r0, #1
	bhi	DelayLoop
	mov	pc, lr

#else

	.align	6

	.globl	DelayLoop
DelayLoop:
	subs	w0, w0, #1
	bne	DelayLoop
	ret

#endif

/* End */
