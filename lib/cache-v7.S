/*
 * cache-v7.S
 *
 *  was: linux/arch/arm/mm/cache-v7.S
 *
 *  Copyright (C) 2001 Deep Blue Solutions Ltd.
 *  Copyright (C) 2005 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  This is the "shell" of the ARMv7 processor support.
 *
 * Modified for Circle by R. Stange
 */
#if RASPPI >= 2

#include <circle/sysconfig.h>

#define CONFIG_PREEMPT

/*
 * From: linux/arch/arm/mm/proc-macros.S
 *
 * dcache_line_size - get the minimum D-cache line size from the CTR register
 * on ARMv7.
 */
	.macro	dcache_line_size, reg, tmp
	mrc	p15, 0, \tmp, c0, c0, 1		@ read ctr
	lsr	\tmp, \tmp, #16
	and	\tmp, \tmp, #0xf		@ cache line size encoding
	mov	\reg, #4			@ bytes per word
	mov	\reg, \reg, lsl \tmp		@ actual cache line size
	.endm

/*
 * The secondary kernel init calls v7_flush_dcache_all before it enables
 * the L1; however, the L1 comes out of reset in an undefined state, so
 * the clean + invalidate performed by v7_flush_dcache_all causes a bunch
 * of cache lines with uninitialized data and uninitialized tags to get
 * written out to memory, which does really unpleasant things to the main
 * processor.  We fix this by performing an invalidate, rather than a
 * clean + invalidate, before jumping into the kernel.
 *
 * This function is cloned from arch/arm/mach-tegra/headsmp.S, and needs
 * to be called for both secondary cores startup and primary core resume
 * procedures.
 */
       .globl  InvalidateDataCacheL1Only
InvalidateDataCacheL1Only:
	push	{r4-r6}
	mov	r0, #0
	mcr	p15, 2, r0, c0, c0, 0
	mrc	p15, 1, r0, c0, c0, 0

	movw	r1, #0x7fff
	and	r2, r1, r0, lsr #13

	movw	r1, #0x3ff

	and	r3, r1, r0, lsr #3		@ NumWays - 1
	add	r2, r2, #1			@ NumSets

	and	r0, r0, #0x7
	add	r0, r0, #4			@ SetShift

	clz	r1, r3				@ WayShift
	add	r4, r3, #1			@ NumWays
1:	sub	r2, r2, #1			@ NumSets--
	mov	r3, r4				@ Temp = NumWays
2:	subs	r3, r3, #1			@ Temp--
	mov	r5, r3, lsl r1
	mov	r6, r2, lsl r0
	orr	r5, r5, r6			@ Reg = (Temp<<WayShift)|(NumSets<<SetShift)
	mcr	p15, 0, r5, c7, c6, 2
	bgt	2b
	cmp	r2, #0
	bgt	1b
	dsb	st
	isb
	pop	{r4-r6}
	bx	lr

/*
 *	was: v7_flush_dcache_all()
 *
 *	Invalidate the whole D-cache.
 *
 *	Corrupted registers: r0-r5, r7, r9-r11
 */
	.globl	InvalidateDataCache
InvalidateDataCache:
	push	{r4-r5, r7, r9-r11}
	dmb					@ ensure ordering with previous memory accesses
	mrc	p15, 1, r0, c0, c0, 1		@ read clidr
	mov	r3, r0, lsr #23			@ move LoC into position
	ands	r3, r3, #7 << 1			@ extract LoC*2 from clidr
	beq	5f				@ if loc is 0, then no need to clean
	mov	r10, #0				@ start clean at cache level 0
1:	add	r2, r10, r10, lsr #1		@ work out 3x current cache level
	mov	r1, r0, lsr r2			@ extract cache type bits from clidr
	and	r1, r1, #7			@ mask of the bits for current cache only
	cmp	r1, #2				@ see what cache we have at this level
	blt	4f				@ skip if no cache, or just i-cache
#ifdef CONFIG_PREEMPT
	mrs	r9, cpsr			@ make cssr&csidr read atomic
	cpsid	i
#endif
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	isb					@ isb to sych the new cssr&csidr
	mrc	p15, 1, r1, c0, c0, 0		@ read the new csidr
#ifdef CONFIG_PREEMPT
	msr	cpsr_c, r9
#endif
	and	r2, r1, #7			@ extract the length of the cache lines
	add	r2, r2, #4			@ add 4 (line length offset)
	movw	r4, #0x3ff
	ands	r4, r4, r1, lsr #3		@ find maximum number on the way size
	clz	r5, r4				@ find bit position of way size increment
	movw	r7, #0x7fff
	ands	r7, r7, r1, lsr #13		@ extract max number of the index size
2:	mov	r9, r7				@ create working copy of max index
3:	orr	r11, r10, r4, lsl r5		@ factor way and cache number into r11
	orr	r11, r11, r9, lsl r2		@ factor index number into r11
	mcr	p15, 0, r11, c7, c6, 2		@ invalidate by set/way
	subs	r9, r9, #1			@ decrement the index
	bge	3b
	subs	r4, r4, #1			@ decrement the way
	bge	2b
4:	add	r10, r10, #2			@ increment cache number
	cmp	r3, r10
	bgt	1b
5:	mov	r10, #0				@ swith back to cache level 0
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	dsb	st
	isb
	pop	{r4-r5, r7, r9-r11}
	bx	lr

/*
 *	Clean the whole D-cache.
 *
 *	Corrupted registers: r0-r5, r7, r9-r11
 */
	.globl	CleanDataCache
CleanDataCache:
	push	{r4-r5, r7, r9-r11}
	dmb					@ ensure ordering with previous memory accesses
	mrc	p15, 1, r0, c0, c0, 1		@ read clidr
	mov	r3, r0, lsr #23			@ move LoC into position
	ands	r3, r3, #7 << 1			@ extract LoC*2 from clidr
	beq	5f				@ if loc is 0, then no need to clean
	mov	r10, #0				@ start clean at cache level 0
1:	add	r2, r10, r10, lsr #1		@ work out 3x current cache level
	mov	r1, r0, lsr r2			@ extract cache type bits from clidr
	and	r1, r1, #7			@ mask of the bits for current cache only
	cmp	r1, #2				@ see what cache we have at this level
	blt	4f				@ skip if no cache, or just i-cache
#ifdef CONFIG_PREEMPT
	mrs	r9, cpsr			@ make cssr&csidr read atomic
	cpsid	i
#endif
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	isb					@ isb to sych the new cssr&csidr
	mrc	p15, 1, r1, c0, c0, 0		@ read the new csidr
#ifdef CONFIG_PREEMPT
	msr	cpsr_c, r9
#endif
	and	r2, r1, #7			@ extract the length of the cache lines
	add	r2, r2, #4			@ add 4 (line length offset)
	movw	r4, #0x3ff
	ands	r4, r4, r1, lsr #3		@ find maximum number on the way size
	clz	r5, r4				@ find bit position of way size increment
	movw	r7, #0x7fff
	ands	r7, r7, r1, lsr #13		@ extract max number of the index size
2:	mov	r9, r7				@ create working copy of max index
3:	orr	r11, r10, r4, lsl r5		@ factor way and cache number into r11
	orr	r11, r11, r9, lsl r2		@ factor index number into r11
	mcr	p15, 0, r11, c7, c10, 2		@ clean by set/way
	subs	r9, r9, #1			@ decrement the index
	bge	3b
	subs	r4, r4, #1			@ decrement the way
	bge	2b
4:	add	r10, r10, #2			@ increment cache number
	cmp	r3, r10
	bgt	1b
5:	mov	r10, #0				@ swith back to cache level 0
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	dsb	st
	isb
	pop	{r4-r5, r7, r9-r11}
	bx	lr

/*
 *	was: v7_flush_kern_dcache_area(void *addr, size_t size)
 *
 *	Ensure that the data held in the page kaddr is written back
 *	to the page in question.
 *
 *	- addr	- kernel address
 *	- size	- region size
 */
	.globl	CleanAndInvalidateDataCacheRange
CleanAndInvalidateDataCacheRange:
	dcache_line_size r2, r3
	add	r1, r0, r1
	sub	r3, r2, #1
	bic	r0, r0, r3
#ifdef CONFIG_ARM_ERRATA_764369
#ifdef ARM_ALLOW_MULTI_CORE
	dsb
#else
	nop
#endif
#endif
1:	mcr	p15, 0, r0, c7, c14, 1		@ clean & invalidate D line / unified line
	add	r0, r0, r2
	cmp	r0, r1
	blo	1b
	dsb	st
	bx	lr

#endif		/* RASPPI >= 2 */

/* End */
