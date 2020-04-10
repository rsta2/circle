#ifndef _p9proc_h
#define _p9proc_h

#include "p9util.h"
#include "p9error.h"
#include <circle/sysconfig.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lock_t
{
#ifdef ARM_ALLOW_MULTI_CORE
	u32int _lock;
#endif
}
Lock;

#define lock	__p9lock
#define unlock	__p9unlock
void lock (Lock *l);
void unlock (Lock *l);

typedef struct qlock_t
{
	volatile int locked;
}
QLock;

void qlock (QLock *qlock);
void qunlock (QLock *qlock);
int canqlock (QLock *qlock);

typedef struct Rendez
{
}
Rendez;

#define sleep	__p9sleep
#define wakeup	__p9wakeup
typedef int sleephandler_t (void *param);
void sleep (Rendez *rendez, sleephandler_t *handler, void *param);
void tsleep (Rendez *rendez, sleephandler_t *handler, void *param, unsigned msecs);
void wakeup (Rendez *rendez);
int return0 (void *param);

#define kproc	__p9kproc
void kproc (const char *name, void (*func) (void *param), void *param);

#define up	__p9up
extern struct up_t
{
	Rendez sleep;
	const char *errstr;
	char genbuf[1000];
	struct error_stack_t errstack;
}
*up;

void p9proc_init (void);

#ifdef __cplusplus
}
#endif

#endif
