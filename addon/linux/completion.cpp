#include <linux/completion.h>
#include <linux/jiffies.h>
#include <linux/bug.h>
#include <circle/sched/scheduler.h>
#include <circle/multicore.h>

void complete (struct completion *x)
{
#ifdef ARM_ALLOW_MULTI_CORE
	BUG_ON (CMultiCoreSupport::ThisCore () != 0);
#endif

	x->done++;
}

void complete_all (struct completion *x)
{
#ifdef ARM_ALLOW_MULTI_CORE
	BUG_ON (CMultiCoreSupport::ThisCore () != 0);
#endif

	x->done = (unsigned) -1 / 2;
}

void wait_for_completion (struct completion *x)
{
#ifdef ARM_ALLOW_MULTI_CORE
	BUG_ON (CMultiCoreSupport::ThisCore () != 0);
#endif

	while (x->done == 0)
	{
		CScheduler::Get ()->Yield ();
	}

	x->done--;
}

int try_wait_for_completion (struct completion *x)
{
#ifdef ARM_ALLOW_MULTI_CORE
	BUG_ON (CMultiCoreSupport::ThisCore () != 0);
#endif

	if (x->done == 0)
	{
		return 0;
	}

	x->done--;

	return 1;
}

long wait_for_completion_interruptible_timeout (struct completion *x, unsigned long timeout)
{
#ifdef ARM_ALLOW_MULTI_CORE
	BUG_ON (CMultiCoreSupport::ThisCore () != 0);
#endif

	unsigned long start = jiffies;

	while (x->done == 0)
	{
		unsigned long now = jiffies;
		if (now-start >= timeout)
		{
			return 0;
		}

		CScheduler::Get ()->Yield ();
	}

	x->done--;

	return 1;
}
