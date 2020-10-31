/*
 * Modified version for the Raspberry Pi 4 which allows using FIQ with Circle
 */
/*
Copyright (c) 2016-2020 Raspberry Pi (Trading) Ltd.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

.arch_extension sec
.arch_extension virt
.syntax unified

#define MACHINE_ID	3138

#if BCM2711
#define OSC_FREQ	54000000
#define LOCAL_BASE	0xff800000
#else
#define OSC_FREQ	19200000
#define LOCAL_BASE	0x40000000
#endif

#define ARML_PRESCALER	0x08
#define ARML_MBOX_CLR03	0xcc

#ifdef GIC
/* Distributor */
#define GIC_DISTB	0x41000
#define GICD_CTLR	0x0
#define GICD_IGROUPR	0x80
#define IT_NR		0x8			/* Number of interrupt enable registers (256 total irqs) */

/* CPU interface */
#define GIC_CPUB	0x42000
#define GICC_CTRLR	0x0
#define GICC_PMR	0x4
#endif

#if BCM2710
#define NSACR_VAL	0xc00
#else
#define NSACR_VAL	0x60c00
#endif

.section .init
.global _start

/* Vector table for secure state and HYP mode */
_start:
	mrc	p15, 0, r6, c0, c0, 5		@ Read MPIDR into r6
	blx	_main				@ Jump to bootstub entrypoint (Thumb-2)

/*
 * Secure monitor handler
 * This is executed on a "smc" instruction, we use a "smc #0" to switch
 * to non-secure state.
 * We only clobber ip (r12) here, so that the caller's r0-r7 remain valid.
 */
_secure_monitor:
#if BCM2710
	mcr	p15, 0, r1, c1, c0, 1		@ Write ACTLR (Secure)
#endif
	mov	ip, #0x131			@ Set NS, AW, FW, HVC
	mcr	p15, 0, ip, c1, c1, 0		@ Write SCR (which sets the NS bit)
	mov	ip, #0x1fa			@ Set HYP_MODE | T_BIT | F_BIT | I_BIT | A_BIT
	msr     spsr_cxfs, ip                   @ Set full SPSR
	movs	pc, lr				@ Return to non-secure HYP

/*
 * Main bootstub entrypoint (entered in secure SVC mode)
 * This code is Thumb-2 in order to save space.
 */
.thumb_func
_main:
	movs	r0, #0				@ Set r0 to zero (used by code below and also as a kernel argument)
	lsls	r6, r6, #30			@ Extract processor number field from MPIDR (shifted left by 30)
	ldr	r7, =LOCAL_BASE			@ Load ARM local peripherals base address for later use

#if !BCM2710
	mrc	p15, 0, r1, c1, c0, 1		@ Read Auxiliary Control Register
	orr	r1, r1, #(1<<6)			@ Enable SMP
	mcr	p15, 0, r1, c1, c0, 1		@ Write Auxiliary Control Register
#else
	mrrc	p15, 1, r1, r2, c15		@ CPU Extended Control Register
	orr	r1, r1, #(1<<6)			@ Enable SMP
#if BCM2711
	bic	r2, r2, #3			@ Set L2 load data prefetch to 0b00 = 16 requests (A72-specific)
#endif
	mcrr	p15, 1, r1, r2, c15		@ CPU Extended Control Register
#endif

	mrc	p15, 0, r1, c1, c0, 0		@ Read System Control Register
	/* Cortex A72 manual 4.3.67 says SMP must be set before enabling the cache. */
	orr	r1, r1, #(1<<12)		@ Enable icache
	mcr	p15, 0, r1, c1, c0, 0		@ Write System Control Register

#ifdef GIC

@ Configure the GIC Distributor
	add	r5, r7, #GIC_DISTB		@ Get base address of GIC_DISTB
	cbnz	r6, 9f				@ Skip setting GICD_CTLR if we are not core 0
	movs	r1, #3
	str	r1, [r5, #GICD_CTLR]		@ Enable group 0 and 1 IRQs from distributor
9:
	mov	r1, #~0				@ Route all interrupts to group 1
	movs	r2, #IT_NR
	add	r5, r5, #GICD_IGROUPR		@ Note: GICD_IGROUPR0 is banked for each core so all cores must set it
0:	str	r1, [r5], #4
	subs	r2, r2, #1
	bne	0b

@ Enable all interrupts coming from group 1 on this core
	add	r5, r7, #GIC_CPUB		@ Get base address of GIC_CPUB
	mov	r1, #0x1e7
	str	r1, [r5, #GICC_CTRLR]		@ Enable group 1 IRQs from CPU interface

	movs	r1, #0xff
	str	r1, [r5, #GICC_PMR]		@ Set priority mask

#endif

@ Allow non-secure access to coprocessors
	ldr	r1, =NSACR_VAL
	mcr	p15, 0, r1, c1, c1, 2		@ NSACR = all copros to non-sec

@ Initialize the architectural timer
	ldr	r1, =OSC_FREQ			@ osc = 19.2 or 54MHz
	mcr	p15, 0, r1, c14, c0, 0		@ Write CNTFRQ
#if BCM2711
	mov	r1, #0x80000000			@ Set ARM_LOCAL_TIMER_PRE_ADD to 1
	str	r1, [r7, #ARML_PRESCALER]
#endif
	movs	r1, #1
	mcr	p15, 0, r1, c14, c3, 1		@ CNTV_CTL (enable=1, imask=0)

@ Initialize exception vectors and switch to non-secure HYP mode
	mcr	p15, 0, r0, c12, c0, 1		@ Set MVBAR to secure vectors (i.e. zero)
	isb
#if BCM2710
	movs	r1, #0x73			@ Value for ACTLR: enable non-secure access to CPUACTLR/CPUECTLR/L2CTLR/L2ECTLR/L2ACTLR
#endif
	smc	#0				@ Call into MONITOR mode
	mcr	p15, 0, r0, c12, c0, 0		@ Write non-secure copy of VBAR
	mcrr	p15, 4, r0, r0, c14		@ Reset CNTVOFF to zero (must be done now in non-secure HYP mode)

#if BCM2711
	movs	r1, #0x22			@ Value for L2CTLR: set L2 read/write latency to 3
	mcr	p15, 1, r1, c9, c0, 2		@ Write L2CTLR
#endif

@ Set kernel entrypoint arguments
	mov	r1, #MACHINE_ID			@ BCM2708 machine id
	ldrd	r2, r3, atags			@ ATAGS and kernel

@ Secondary cores only: synchronize with core 0 using the mailbox
	cbz	r6, 9f				@ Skip this section if we are core 0
	lsrs	r6, r6, #(30-4)			@ Calculate offset to mailbox register (i.e. ARML_MBOX_CLR03 + coreid*0x10)
	adds	r6, r6, #ARML_MBOX_CLR03
0:	wfe
	ldr	r3, [r7, r6]			@ Read message (secondary kernel entrypoint)
	cmp	r3, #0				@ If zero, there is no message
	beq	0b
	str	r3, [r7, r6]			@ Clear mailbox
9:

@ Jump to kernel
	bx	r3

@ Assembler generated constant pool goes here
.pool

.org 0xd4
	.ascii "FIQS"	@ Circle GIC code checks this for presence

.org 0xf0
.word 0x5afe570b	@ magic value to indicate firmware should overwrite atags and kernel
.word 0			@ version
atags:	.word 0x0	@ device tree address
kernel:	.word 0x0	@ kernel start address
