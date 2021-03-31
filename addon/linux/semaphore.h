#ifndef _linux_semaphore_h
#define _linux_semaphore_h

#include <linux/atomic.h>
#include <linux/compiler.h>

#ifdef __cplusplus
extern "C" {
#endif

struct semaphore
{
	atomic_t count;
};

#define DEFINE_SEMAPHORE(name)	struct semaphore name = {ATOMIC_INIT (1)}

static inline void sema_init (struct semaphore *sem, int val)
{
	atomic_set (&sem->count, val);
}

void down (struct semaphore *sem);
void up (struct semaphore *sem);

// returns 0 if semaphore has been locked, 1 otherwise
int __must_check down_trylock (struct semaphore *sem);

#ifdef __cplusplus
}
#endif

#endif
