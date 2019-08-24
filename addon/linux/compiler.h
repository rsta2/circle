#ifndef _linux_compiler_h
#define _linux_compiler_h

#ifdef __GNUC__
	#define __must_check	__attribute__ ((warn_unused_result))

	#define likely(exp)	__builtin_expect (!!(exp), 1)
	#define unlikely(exp)	__builtin_expect (!!(exp), 0)
#else
	#define __must_check

	#define likely(exp)	(exp)
	#define unlikely(exp)	(exp)
#endif

#define __force

#define __user
#define __iomem

#endif
