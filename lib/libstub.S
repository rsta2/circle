/*
 * libstub.S
 */

	.text

	.globl	__aeabi_uidiv
__aeabi_uidiv:
	push	{lr}
	mov	r2, #0
	bl	__Divide
	pop	{pc}

	.globl	__aeabi_uidivmod
__aeabi_uidivmod:
	push	{lr}
	sub	sp, sp, #12
	add	r2, sp, #4
	bl	__Divide
	ldr	r1, [sp, #4]
	add	sp, sp, #12
	pop	{pc}

	.globl	__aeabi_idiv
__aeabi_idiv:
	b	__DivideInteger
	
	.global __aeabi_idivmod
__aeabi_idivmod:
	stmfd	sp!, {r0, r1, ip, lr}
	bl	__aeabi_idiv
	ldmfd	sp!, {r1, r2, ip, lr}
	mul	r3, r0, r2
	sub	r1, r1, r3
	mov	pc, lr

.equ  locked,   1
.equ  unlocked, 0
	
// BUG: lock_mutex hangs
// LDREX doesn't work when MMU is disabled. Don't use this.
// Declare for use from C as extern void lock_mutex(void * mutex);
.global _lock_mutex_mmu
_lock_mutex_mmu:
  LDR     r1, =locked
1:
  LDREX   r2, [r0]
  CMP     r2, r1        // Test if mutex is locked or unlocked
  BEQ     2f
  STREXNE r2, r1, [r0]  // Not locked, attempt to lock it
  CMPNE   r2, #1        // Check if Store-Exclusive failed
  BEQ     1b           // Failed - retry from 1
  // Lock acquired
//  DMB                   // Required before accessing protected resource
  BX      lr
2:
// Take appropriate action while waiting for mutex to become unlocked
  //wfi
  nop
  B       1b           // Retry from 1


// BUG: unlock_mutex
// Declare for use from C as extern void unlock_mutex(void * mutex);
.global _unlock_mutex_mmu
_unlock_mutex_mmu:
    LDR     r1, =unlocked
//    DMB                   // Required before releasing protected resource
    STR     r1, [r0]      // Unlock mutex
    // SIGNAL_UPDATE: none
    BX      lr


// BUG: SWP version
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dht0008a/CJHBGBBJ.html
// still hangs
.global _lock_mutex_swp
_lock_mutex_swp:
    LDR r2, =locked
    SWP r1, r2, [r0]       // Swap R2 with location [R0], [R0] value placed in R1
    CMP r1, r2             // Check if memory value was ‘locked’
    BEQ _lock_mutex_swp     // If so, retry immediately
    BX  lr                 // If not, lock successful, return

// BUG: not really excusive when context swithes after ldr befor str
.global _lock_mutex_simple
_lock_mutex_simple:
  ldr r1, =unlocked
  ldr r3, =locked
  ldr r2, [r0]
  cmp r2, r3
  beq _lock_mutex_simple
  str r1, [r0]
  bx lr

.global _unlock_mutex_simple
_unlock_mutex_simple:
    LDR r1, =unlocked
    STR r1, [r0]           // Write value ‘unlocked’ to location [R0]
    BX  lr	

/* End */
