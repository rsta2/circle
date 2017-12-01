#ifndef _linux_completion_h
#define _linux_completion_h

#ifdef __cplusplus
extern "C" {
#endif

struct completion
{
	volatile int done;
};

static inline void init_completion (struct completion *x)
{
	x->done = 0;
}

static inline void reinit_completion (struct completion *x)
{
	x->done = 0;
}

void complete (struct completion *x);
void complete_all (struct completion *x);

void wait_for_completion (struct completion *x);

static inline int wait_for_completion_interruptible (struct completion *x)
{
	wait_for_completion (x);

	return 0;
}

static inline int wait_for_completion_killable (struct completion *x)
{
	wait_for_completion (x);

	return 0;
}

// returns 1 on success, 0 otherwise
int try_wait_for_completion (struct completion *x);

// timeout in jiffies
// returns > 0 on success, 0 on timeout, < 0 on error
long wait_for_completion_interruptible_timeout (struct completion *x, unsigned long timeout);

#ifdef __cplusplus
}
#endif

#endif
