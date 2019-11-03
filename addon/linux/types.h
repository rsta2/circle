#ifndef _linux_types_h
#define _linux_types_h

#include <circle/types.h>
#include <stdint.h>

typedef unsigned long	loff_t;
typedef int		gfp_t;
typedef uintptr		dma_addr_t;

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifndef NULL
#define NULL	0
#endif

struct list_head
{
	struct list_head *next;
	struct list_head *prev;
};

#endif
