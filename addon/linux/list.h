#ifndef _linux_list_h
#define _linux_list_h

#include <linux/kernel.h>
#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void INIT_LIST_HEAD (struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline void list_add (struct list_head *elem, struct list_head *head)
{
	struct list_head *next = head->next;

	next->prev = elem;
	elem->next = next;
	elem->prev = head;
	head->next = elem;
}

static inline void list_add_tail (struct list_head *elem, struct list_head *head)
{
	struct list_head *prev = head->prev;

	head->prev = elem;
	elem->next = head;
	elem->prev = prev;
	prev->next = elem;
}

static inline void list_del (struct list_head *entry)
{
	struct list_head *prev = entry->prev;
	struct list_head *next = entry->next;

	next->prev = prev;
	prev->next = next;
}

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

#ifdef __cplusplus
}
#endif

#endif
