#ifndef _linux_uaccess_h
#define _linux_uaccess_h

#include <linux/compiler.h>
#include <circle/util.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline unsigned long copy_from_user (void *to, const void __user *from, unsigned long len)
{
	memcpy (to, from, len);

	return 0;
}

static inline unsigned long copy_to_user (void __user *to, const void *from, unsigned long len)
{
	memcpy (to, from, len);

	return 0;
}

#ifdef __cplusplus
}
#endif

#endif
