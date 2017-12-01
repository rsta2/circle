#ifndef _linux_pagemap_h
#define _linux_pagemap_h

#include <circle/sysconfig.h>

#define PAGE_ALIGN(val)		(((val) + PAGE_SIZE-1) & ~(PAGE_SIZE-1))

#endif
