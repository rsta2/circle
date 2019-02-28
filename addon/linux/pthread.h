/*
 * Information about these definitions has been taken from the GNU C Library,
 * which is licensed under the GNU Lesser General Public License version 2.1.
 */
#ifndef _linux_pthread_h
#define _linux_pthread_h

#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Types */

typedef unsigned int pthread_key_t;

typedef struct pthread
{
	struct task_struct *kthread;
	void *(*start_func) (void *);
	void *start_arg;
	void *retval;
	struct list_head key_list;
}
*pthread_t;

typedef struct {} pthread_attr_t;

typedef struct {} pthread_mutexattr_t;
#define PTHREAD_MUTEX_RECURSIVE		(0)

typedef struct mutex pthread_mutex_t;
#define PTHREAD_MUTEX_INITIALIZER	{0}

typedef int pthread_once_t;
#define PTHREAD_ONCE_INIT		(0)

/* Functions */

int pthread_attr_init (pthread_attr_t *attr);
int pthread_attr_destroy (pthread_attr_t *attr);
int pthread_attr_setstacksize (pthread_attr_t *attr, size_t stacksize);

int pthread_create (pthread_t *thread, const pthread_attr_t *attr,
		    void *(*func) (void *), void *arg);

pthread_t pthread_self (void);

void pthread_exit (void *retval);
int pthread_join (pthread_t thread, void **retval);

int pthread_mutexattr_init (pthread_mutexattr_t *attr);
int pthread_mutexattr_destroy (pthread_mutexattr_t *attr);
int pthread_mutexattr_settype (pthread_mutexattr_t *attr, int type);

int pthread_mutex_init (pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_destroy (pthread_mutex_t *mutex);
int pthread_mutex_lock (pthread_mutex_t *mutex);
int pthread_mutex_trylock (pthread_mutex_t *mutex);
int pthread_mutex_unlock (pthread_mutex_t *mutex);

int pthread_key_create (pthread_key_t *key, void (*destructor) (void *));
int pthread_key_delete (pthread_key_t key);
int pthread_setspecific (pthread_key_t key, const void *value);
void *pthread_getspecific (pthread_key_t key);

int pthread_once (pthread_once_t *control, void (*func) (void));

#ifdef __cplusplus
}
#endif

#endif
