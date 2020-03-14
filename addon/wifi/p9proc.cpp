#include "p9proc.h"
#include "p9arch.h"
#include <circle/timer.h>
#include <circle/synchronize.h>
#include <circle/sched/scheduler.h>
#include <circle/sched/task.h>
#include <circle/sysconfig.h>
#include <assert.h>

#ifdef ARM_ALLOW_MULTI_CORE					// TODO
	#error ARM_ALLOW_MULTI_CORE is not supported!
#endif

void lock (Lock *lock)
{
	EnterCritical (IRQ_LEVEL);
}

void unlock (Lock *lock)
{
	LeaveCritical ();
}

void qlock (QLock *qlock)
{
	do
	{
		CScheduler::Get ()->Yield ();
	}
	while (qlock->locked);

	qlock->locked = 1;
}

void qunlock (QLock *qlock)
{
	assert (qlock->locked);
	qlock->locked = 0;

	CScheduler::Get ()->Yield ();
}

int canqlock (QLock *qlock)
{
	if (qlock->locked)
	{
		CScheduler::Get ()->Yield ();

		return 0;
	}

	qlock->locked = 1;

	return 1;
}

void sleep (Rendez *rendez, sleephandler_t *handler, void *param)
{
	do
	{
		CScheduler::Get ()->Yield ();
	}
	while ((*handler) (param) == 0);
}

void tsleep (Rendez *rendez, sleephandler_t *handler, void *param, unsigned msecs)
{
	unsigned start = m->ticks;
	do
	{
		if (m->ticks-start > msecs*HZ/1000)
		{
			break;
		}

		CScheduler::Get ()->Yield ();
	}
	while ((*handler) (param) == 0);
}

void wakeup (Rendez *rendez)
{
}

int return0 (void *param)
{
	return 0;
}

class CKProc : public CTask
{
public:
	CKProc (void (*procfn) (void *param), void *param)
	:	m_procfn (procfn),
		m_param (param)
	{
	}

	void Run (void)
	{
		(*m_procfn) (m_param);
	}

private:
	void (*m_procfn) (void *param);
	void *m_param;
};

void kproc (const char *name, void (*func) (void *), void *parm)
{
	new CKProc (func, parm);
}

static struct up_t upstruct;
struct up_t *up = &upstruct;
