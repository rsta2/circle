#include <linux/delay.h>
#include <circle/timer.h>
#include <circle/sched/scheduler.h>

void udelay (unsigned long usecs)
{
	CTimer::Get ()->usDelay (usecs);
}

void msleep (unsigned msecs)
{
	CScheduler::Get ()->MsSleep (msecs);
}
