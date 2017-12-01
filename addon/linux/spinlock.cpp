#include <linux/spinlock.h>
#include <circle/synchronize.h>
#include <circle/multicore.h>

#define SPINLOCK_SAVE_POWER

#ifdef ARM_ALLOW_MULTI_CORE

void spin_lock (spinlock_t *lock)
{
	asm volatile
	(
		"mrs %0, cpsr\n"
		"cpsid i\n"

		: "=r" (lock->cpsr[CMultiCoreSupport::ThisCore ()])
	);

	// See: ARMv7-A Architecture Reference Manual, Section D7.3
	asm volatile
	(
		"mov r1, %0\n"
		"mov r2, #1\n"
		"1: ldrex r3, [r1]\n"
		"cmp r3, #0\n"
#ifdef SPINLOCK_SAVE_POWER
		"wfene\n"
#endif
		"strexeq r3, r2, [r1]\n"
		"cmpeq r3, #0\n"
		"bne 1b\n"
		"dmb\n"

		: : "r" ((uintptr) &lock->lock)
	);
}

void spin_unlock (spinlock_t *lock)
{
	// See: ARMv7-A Architecture Reference Manual, Section D7.3
	asm volatile
	(
		"mov r1, %0\n"
		"mov r2, #0\n"
		"dmb\n"
		"str r2, [r1]\n"
#ifdef SPINLOCK_SAVE_POWER
		"dsb\n"
		"sev\n"
#endif

		: : "r" ((uintptr) &lock->lock)
	);

	asm volatile
	(
		"msr cpsr_c, %0\n"

		: : "r" (lock->cpsr[CMultiCoreSupport::ThisCore ()])
	);
}

#else

void spin_lock (spinlock_t *lock)
{
	EnterCritical (IRQ_LEVEL);
}

void spin_unlock (spinlock_t *lock)
{
	LeaveCritical ();
}

#endif
