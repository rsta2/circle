#ifndef _linux_mutex_h
#define _linux_mutex_h

#ifdef __cplusplus
extern "C" {
#endif

struct mutex
{
	volatile int lock;
};

static inline void mutex_init (struct mutex *lock)
{
	lock->lock = 0;
}

void mutex_lock (struct mutex *lock);
void mutex_unlock (struct mutex *lock);

static inline int mutex_lock_interruptible (struct mutex *lock)
{
	mutex_lock (lock);

	return 0;
}

#ifdef __cplusplus
}
#endif

#endif
