/*
 * startup.S
 */
#include <circle/sysconfig.h>

	.text

	.globl	_start
_start:
	cps	#0x1F				/* set system mode */
	mov	sp, #MEM_KERNEL_STACK
	b	sysinit

/* End */
