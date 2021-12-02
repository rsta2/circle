#include <linux/pthread.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/bug.h>
#include <circle/sysconfig.h>
#include <circle/multicore.h>
#include <circle/sched/task.h>

static pthread_key_t next_key = 1;

struct pthread_key_element
{
	struct list_head head;
	pthread_key_t key;
	void *value;
};

int pthread_attr_init (pthread_attr_t *attr)
{
	return 0;
}

int pthread_attr_destroy (pthread_attr_t *attr)
{
	return 0;
}

int pthread_attr_setstacksize (pthread_attr_t *attr, size_t stacksize)
{
	return 0;
}

static int _start (void *arg)
{
	struct pthread *thread = (struct pthread *) arg;

	thread->retval = (*thread->start_func) (thread->start_arg);

	return 0;
}

int pthread_create (pthread_t *thread, const pthread_attr_t *attr,
		    void *(*func) (void *), void *arg)
{
	struct pthread *p = (struct pthread *) kmalloc (sizeof (struct pthread), GFP_KERNEL);
	if (p == 0)
	{
		return -ENOMEM;
	}

	p->start_func = func;
	p->start_arg = arg;
	p->retval = 0;
	INIT_LIST_HEAD (&p->key_list);

	p->kthread = kthread_create (_start, p, "pthread@%lp", p);
	BUG_ON (p->kthread == 0);

	p->kthread->userdata = p;

	BUG_ON (thread == 0);
	*thread = p;

	return 0;
}

pthread_t pthread_self (void)
{
	BUG_ON (current == 0);

	if (current->userdata == 0)
	{
		struct pthread *p = (struct pthread *) kmalloc (sizeof (struct pthread), GFP_KERNEL);
		BUG_ON (p == 0);

		p->retval = 0;
		INIT_LIST_HEAD (&p->key_list);
		p->kthread = current;

		current->userdata = p;
	}

	return (pthread_t) current->userdata;
}

void pthread_exit (void *retval)
{
	struct pthread *p = pthread_self ();
	BUG_ON (p == 0);

	p->retval = retval;

	BUG_ON (p->kthread == 0);
	BUG_ON (p->kthread->terminated);

	CTask *ctask = (CTask *) p->kthread->taskobj;
	BUG_ON (ctask == 0);
	ctask->Terminate ();

	BUG ();
}

// TODO: call destructors on thread->key_list and destroy thread->key_list
int pthread_join (pthread_t thread, void **retval)
{
	BUG_ON (thread == 0);
	BUG_ON (thread->kthread == 0);

	if (!thread->kthread->terminated)
	{
		CTask *ctask = (CTask *) thread->kthread->taskobj;
		BUG_ON (ctask == 0);

		ctask->WaitForTermination ();
	}

	if (retval != 0)
	{
		*retval = thread->retval;
	}

	kfree (thread->kthread);
	kfree (thread);

	return 0;
}

int pthread_mutexattr_init (pthread_mutexattr_t *attr)
{
	return 0;
}

int pthread_mutexattr_destroy (pthread_mutexattr_t *attr)
{
	return 0;
}

int pthread_mutexattr_settype (pthread_mutexattr_t *attr, int type)
{
	return 0;
}

int pthread_mutex_init (pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
	mutex_init (mutex);

	return 0;
}

int pthread_mutex_destroy (pthread_mutex_t *mutex)
{
	BUG_ON (mutex->lock != 0);

	return 0;
}

int pthread_mutex_lock (pthread_mutex_t *mutex)
{
	mutex_lock (mutex);

	return 0;
}

int pthread_mutex_trylock (pthread_mutex_t *mutex)
{
#ifdef ARM_ALLOW_MULTI_CORE
	BUG_ON (CMultiCoreSupport::ThisCore () != 0);
#endif

	if (mutex->lock != 0)
	{
		return -EAGAIN;
	}

	mutex->lock = 1;

	return 0;
}

int pthread_mutex_unlock (pthread_mutex_t *mutex)
{
	mutex_unlock (mutex);

	return 0;
}

int pthread_key_create (pthread_key_t *key, void (*destructor) (void *))
{
	BUG_ON (next_key == 0);
	*key = next_key++;

	return 0;
}

// TODO: delete specific values associated with this key for all threads
int pthread_key_delete (pthread_key_t key)
{
	return 0;
}

int pthread_setspecific (pthread_key_t key, const void *value)
{
	pthread_t thread = pthread_self ();
	BUG_ON (thread == 0);

	struct list_head *elem;
	list_for_each (elem, &thread->key_list)
	{
		struct pthread_key_element *key_element =
			list_entry (elem, struct pthread_key_element, head);

		if (key_element->key == key)
		{
			key_element->value = (void *) value;

			return 0;
		}
	}

	struct pthread_key_element *key_element = new struct pthread_key_element;
	BUG_ON (key_element == 0);

	key_element->key = key;
	key_element->value = (void *) value;

	list_add_tail (&key_element->head, &thread->key_list);

	return 0;
}

void *pthread_getspecific (pthread_key_t key)
{
	pthread_t thread = pthread_self ();
	BUG_ON (thread == 0);

	struct list_head *elem;
	list_for_each (elem, &thread->key_list)
	{
		struct pthread_key_element *key_element =
			list_entry (elem, struct pthread_key_element, head);

		if (key_element->key == key)
		{
			return key_element->value;
		}
	}

	return 0;
}

int pthread_once (pthread_once_t *control, void (*func) (void))
{
	if (*control == PTHREAD_ONCE_INIT)
	{
		*control = 1;

		(*func) ();
	}

	return 0;
}
