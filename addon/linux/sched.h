#ifndef _linux_sched_h
#define _linux_sched_h

#ifdef __cplusplus
extern "C" {
#endif

struct task_struct
{
	int pid;
	int terminated;
	void *taskobj;		// opaque pointer
	void *userdata;
};

extern struct task_struct *current;

void set_user_nice (struct task_struct *task, long nice);

int wake_up_process (struct task_struct *task);

void flush_signals (struct task_struct *task);

static inline int fatal_signal_pending (struct task_struct *task)
{
	return 0;
}

#ifdef __cplusplus
}
#endif

#endif
