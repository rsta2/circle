#ifndef _linux_bug_h
#define _linux_bug_h

#include <linux/compiler.h>
#include <linux/printk.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BUG_ON(exp)	assert (!(exp))
#define BUG()		assert (0)

void __warn (const char *file, const int line);
#define __WARN()	__warn (__FILE__, __LINE__)

#define WARN_ON(condition) ({				\
	int __ret_warn_on = !!(condition);		\
	if (unlikely(__ret_warn_on))			\
		__WARN();				\
	unlikely(__ret_warn_on);			\
})

#define WARN(condition, format...) ({			\
	int __ret_warn_on = !!(condition);		\
	if (unlikely(__ret_warn_on))			\
		printk(format);				\
	unlikely(__ret_warn_on);			\
})

#define WARN_ON_ONCE(condition)	({			\
	static int __warned;				\
	int __ret_warn_once = !!(condition);		\
	if (unlikely(__ret_warn_once && !__warned)) {	\
		__warned = 1;				\
		__WARN();				\
	}						\
	unlikely(__ret_warn_once);			\
})

#define WARN_ONCE(condition, format...)	({		\
	static int __warned;				\
	int __ret_warn_once = !!(condition);		\
	if (unlikely(__ret_warn_once && !__warned)) {	\
		__warned = 1;				\
		WARN(1, format);			\
	}						\
	unlikely(__ret_warn_once);			\
})

#ifdef __cplusplus
}
#endif

#endif
