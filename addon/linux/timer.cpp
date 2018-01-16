#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/spinlock.h>
#include <circle/timer.h>

unsigned long volatile jiffies = 0;

static struct list_head timer_list;
static struct spinlock timer_lock;

unsigned long msecs_to_jiffies (const unsigned int msecs)
{
	return MSEC2HZ (msecs);
}

void init_timer (struct timer_list *timer)
{
	INIT_LIST_HEAD (&timer->entry);

	timer->expires = 0;
	timer->function = 0;
	timer->data = 0;
}

void add_timer (struct timer_list *timer)
{
	spin_lock (&timer_lock);

	unsigned long expires = timer->expires;

	struct list_head *pos;
	list_for_each (pos, &timer_list)
	{
		struct timer_list *t = list_entry (pos, struct timer_list, entry);
		if ((long) (t->expires-expires) < 0)
		{
			break;
		}
	}

	list_add (&timer->entry, pos);

	spin_unlock (&timer_lock);
}

int del_timer (struct timer_list *timer)
{
	int ret = 0;

	spin_lock (&timer_lock);

	if (timer->entry.next != &timer->entry)
	{
		list_del (&timer->entry);

		ret = 1;
	}

	spin_unlock (&timer_lock);

	if (ret)
	{
		INIT_LIST_HEAD (&timer->entry);
	}

	return ret;
}

static void periodic_handler (void)
{
	spin_lock (&timer_lock);

	++jiffies;
	unsigned long now = jiffies;

	int cont;
	do
	{
		cont = 0;

		struct list_head *pos = timer_list.next;
		if (pos != &timer_list)
		{
			struct timer_list *t = list_entry (pos, struct timer_list, entry);
			if ((long) (t->expires-now) <= 0)
			{
				list_del (&t->entry);

				spin_unlock (&timer_lock);

				INIT_LIST_HEAD (&t->entry);

				(*t->function) (t->data);

				cont = 1;

				spin_lock (&timer_lock);
			}
		}
	}
	while (cont);

	spin_unlock (&timer_lock);
}

int linuxemu_init_timer (void)
{
	INIT_LIST_HEAD (&timer_list);
	spin_lock_init (&timer_lock);

	CTimer::Get ()->RegisterPeriodicHandler (periodic_handler);

	return 0;
}
