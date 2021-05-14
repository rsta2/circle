#include <linux/semaphore.h>
#include <linux/bug.h>
#include <circle/sched/scheduler.h>
#include <circle/multicore.h>

void down (struct semaphore *sem)
{
#ifdef ARM_ALLOW_MULTI_CORE
	BUG_ON (CMultiCoreSupport::ThisCore () != 0);
#endif

	while (atomic_read (&sem->count) == 0)
	{
		CScheduler::Get ()->Yield ();
	}

	atomic_dec (&sem->count);
}

void up (struct semaphore *sem)
{
#ifdef ARM_ALLOW_MULTI_CORE
	BUG_ON (CMultiCoreSupport::ThisCore () != 0);
#endif

	atomic_inc (&sem->count);
}

int down_trylock (struct semaphore *sem)
{
#ifdef ARM_ALLOW_MULTI_CORE
	BUG_ON (CMultiCoreSupport::ThisCore () != 0);
#endif

	if (atomic_read (&sem->count) == 0)
	{
		return 1;
	}

	atomic_dec (&sem->count);

	return 0;
}
