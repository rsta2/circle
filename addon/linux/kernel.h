#ifndef _linux_kernel_h
#define _linux_kernel_h

#include <linux/compiler.h>
#include <linux/printk.h>
#include <linux/bug.h>
#include <linux/barrier.h>
#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef offsetof
#ifdef __GNUC__
	#define offsetof(type, member)	__builtin_offsetof (type, member)
#else
	#define offsetof(type, member)	((size_t)&((type *)0)->member)
#endif
#endif

#define container_of(ptr, type, member) \
	((type *) ((char *) (ptr) - offsetof (type, member)))

#ifndef min
#define min(a, b)	((a) < (b) ? (a) : (b))
#endif

#define sprintf		linuxemu_sprintf
#define snprintf	linuxemu_snprintf
int sprintf (char *buf, const char *fmt, ...);
int snprintf (char *buf, size_t size, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
