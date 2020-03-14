#ifndef _p9proc_h
#define _p9proc_h

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lock_t
{
}
Lock;

void lock (Lock *lock);
void unlock (Lock *lock);

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

typedef int sleephandler_t (void *param);
void sleep (Rendez *rendez, sleephandler_t *handler, void *param);
void tsleep (Rendez *rendez, sleephandler_t *handler, void *param, unsigned msecs);
void wakeup (Rendez *rendez);
int return0 (void *param);

void kproc (const char *name, void (*func) (void *param), void *param);

extern struct up_t
{
	Rendez sleep;
	const char *errstr;
	char genbuf[1000];
}
*up;

#ifdef __cplusplus
}
#endif

#endif
