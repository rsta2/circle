#ifndef _linux_timer_h
#define _linux_timer_h

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/list.h>

struct timer_list
{
	struct list_head entry;
	unsigned long	 expires;
	void		 (*function)(unsigned long data);
	unsigned long	 data;
};

void init_timer (struct timer_list *timer);

void add_timer (struct timer_list *timer);
int del_timer (struct timer_list *timer);

int linuxemu_init_timer (void);

#ifdef __cplusplus
}
#endif

#endif
