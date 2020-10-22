/*
 * glibcbind.S
 *
 * If USE_GLIBC_HACK is defined, this file provides the bindings required to use
 * math functions from GLIBC. This can be used to build Circle applications, which
 * use such functions, on Raspberry Pi OS. This has been tested with GLIBC 2.28.
 * Because it is not completely clear, if this works under all circumstances, it is
 * called a hack and should not be used for production.
 */

#if defined (USE_GLIBC_HACK) && STDLIB_SUPPORT == 1

	.text

	.globl	__aeabi_read_tp
__aeabi_read_tp:
	ldr	r0, =tp
	bx	lr


	.bss
	
	.align	4
	
	.space	1024
tp:
	.space	1024


	.section .tbss
	
	.align	2

	.globl	errno
errno:
	.word	0

#endif

/* End */
