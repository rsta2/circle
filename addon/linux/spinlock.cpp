#include <linux/spinlock.h>
#include <circle/synchronize.h>
#include <circle/multicore.h>

#define SPINLOCK_SAVE_POWER

#ifdef ARM_ALLOW_MULTI_CORE

void spin_lock (spinlock_t *lock)
{
	EnterCritical (IRQ_LEVEL);

#if AARCH == 32
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

		: : "r" ((uintptr) &lock->lock) : "r1", "r2", "r3"
	);
#else
	// See: ARMv8-A Architecture Reference Manual, Section K10.3.1
	asm volatile
	(
		"mov x1, %0\n"
		"mov w2, #1\n"
		"prfm pstl1keep, [x1]\n"
		"1: ldaxr w3, [x1]\n"
		"cbnz w3, 1b\n"
		"stxr w3, w2, [x1]\n"
		"cbnz w3, 1b\n"

		: : "r" ((uintptr) &lock->lock) : "x1", "x2", "x3"
	);
#endif
}

void spin_unlock (spinlock_t *lock)
{
#if AARCH == 32
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

		: : "r" ((uintptr) &lock->lock) : "r1", "r2"
	);
#else
	// See: ARMv8-A Architecture Reference Manual, Section K10.3.2
	asm volatile
	(
		"mov x1, %0\n"
		"stlr wzr, [x1]\n"

		: : "r" ((uintptr) &lock->lock) : "x1"
	);
#endif

	LeaveCritical ();
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
