/*
 * libstub.S
 */

	.text

	.globl	__aeabi_uidiv
__aeabi_uidiv:
	push	{r3, lr}
	mov	r2, #0
	bl	Divide
	pop	{r3, pc}

	.globl	__aeabi_uidivmod
__aeabi_uidivmod:
	push	{lr}
	sub	sp, sp, #12
	add	r2, sp, #4
	bl	Divide
	ldr	r1, [sp, #4]
	add	sp, sp, #12
	pop	{pc}

/* End */
