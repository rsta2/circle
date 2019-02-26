#ifndef _linux_kthread_h
#define _linux_kthread_h

#include <linux/sched.h>

#ifdef __cplusplus
extern "C" {
#endif

struct task_struct *kthread_create (int (*threadfn)(void *data),
				    void *data,
				    const char namefmt[], ...);

int linuxemu_init_kthread (void);

#ifdef __cplusplus
}
#endif

#endif
