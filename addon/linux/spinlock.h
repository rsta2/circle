#ifndef _linux_spinlock_h
#define _linux_spinlock_h

#include <linux/rwlock.h>
#include <linux/types.h>
#include <circle/sysconfig.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct spinlock
{
#ifdef ARM_ALLOW_MULTI_CORE
	u32 lock;
#endif
}
spinlock_t;

#ifdef ARM_ALLOW_MULTI_CORE
	#define DEFINE_SPINLOCK(name)		struct spinlock name = {0}
#else
	#define DEFINE_SPINLOCK(name)		struct spinlock name
#endif

static inline void spin_lock_init (spinlock_t *lock)
{
#ifdef ARM_ALLOW_MULTI_CORE
	lock->lock = 0;
#endif
}

void spin_lock (spinlock_t *lock);
void spin_unlock (spinlock_t *lock);

#ifdef __cplusplus
}
#endif

#endif
