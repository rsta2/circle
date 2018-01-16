#ifndef _linux_compiler_h
#define _linux_compiler_h

#ifdef __GNUC__
	#define __must_check	__attribute__ ((warn_unused_result))
#else
	#define __must_check
#endif

#define __force

#define likely(exp)		(exp)
#define unlikely(exp)		(exp)

#define __user
#define __iomem

#endif
