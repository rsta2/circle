#include "p9proc.h"
#include "p9arch.h"
#include <circle/timer.h>
#include <circle/synchronize.h>
#include <circle/sched/scheduler.h>
#include <circle/sched/task.h>
#include <assert.h>

#define SPINLOCK_SAVE_POWER

#ifdef ARM_ALLOW_MULTI_CORE

void lock (Lock *l)
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

		: : "r" ((uintptr) &l->_lock) : "r1", "r2", "r3"
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

		: : "r" ((uintptr) &l->_lock) : "x1", "x2", "x3"
	);
#endif
}

void unlock (Lock *l)
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

		: : "r" ((uintptr) &l->_lock) : "r1", "r2"
	);
#else
	// See: ARMv8-A Architecture Reference Manual, Section K10.3.2
	asm volatile
	(
		"mov x1, %0\n"
		"stlr wzr, [x1]\n"

		: : "r" ((uintptr) &l->_lock) : "x1"
	);
#endif

	LeaveCritical ();
}

#else

void lock (Lock *l)
{
	EnterCritical (IRQ_LEVEL);
}

void unlock (Lock *l)
{
	LeaveCritical ();
}

#endif

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
	CKProc (void (*procfn) (void *param), void *param, const char *name)
	:	m_procfn (procfn),
		m_param (param)
	{
		SetName (name);
	}

	void Run (void)
	{
		m_errstack.stackptr = ERROR_STACK_SIZE;
		SetUserData (&m_errstack, TASK_USER_DATA_ERROR_STACK);

		(*m_procfn) (m_param);
	}

private:
	void (*m_procfn) (void *param);
	void *m_param;

	struct error_stack_t m_errstack;
};

void kproc (const char *name, void (*func) (void *), void *parm)
{
	new CKProc (func, parm, name);
}

static struct up_t upstruct;
struct up_t *up = &upstruct;

void p9proc_init (void)
{
	up->errstr = "";

	up->errstack.stackptr = ERROR_STACK_SIZE;
	CTask *pTask = CScheduler::Get ()->GetCurrentTask ();
	assert (pTask != 0);
	pTask->SetUserData (&up->errstack, TASK_USER_DATA_ERROR_STACK);
}

struct error_stack_t *get_error_stack (void)
{
	CTask *pTask = CScheduler::Get ()->GetCurrentTask ();
	assert (pTask != 0);

	struct error_stack_t *errstack =
		(struct error_stack_t *) pTask->GetUserData (TASK_USER_DATA_ERROR_STACK);
	if (errstack == 0)
	{
		errstack = &up->errstack;
	}

	return errstack;
}
