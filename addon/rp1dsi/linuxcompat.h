#ifndef _rp1dsi_linuxcompat_h
#define _rp1dsi_linuxcompat_h

#include <circle/sched/scheduler.h>
#include <circle/sysconfig.h>
#include <circle/alloc.h>
#include <circle/timer.h>
#include <circle/types.h>

#ifndef NULL
	#define NULL	0
#endif

#ifndef EINVAL
	#define EINVAL	1
#endif

#ifndef EIO
	#define EIO	2
#endif

#ifndef ENOSYS
	#define ENOSYS	3
#endif

#ifndef ENOMEM
	#define ENOMEM	4
#endif

#ifndef EPROTO
	#define EPROTO	5
#endif

#ifndef ENOMSG
	#define ENOMSG	6
#endif

#define ARRAY_SIZE(a)			(sizeof(a) / sizeof ((a)[0]))

#define min(n, m)			((n) < (m) ? (n) : (m))
#define max(n, m)			((n) > (m) ? (n) : (m))

#define clamp(val, lo, hi)		((val) >= (hi) ? (hi) : ((val) <= (lo) ? (lo) : (val)))
#define clamp_val			clamp

#define mul_u32_u32(n, m)		((u64)(n) * (u64)(m))
#define div_u64(n, m)			((u64)(n) / (u64)(m))
#define DIV_U64_ROUND_CLOSEST(n, m)	(((u64)(n) + (u64)(m)/2) / (u64)(m))

#define EXPORT_SYMBOL(s)

#ifndef NO_BUSY_WAIT

#define kmalloc(size, flags)		malloc (size)
#define kfree(ptr)			free (ptr)

static inline void usleep_range(unsigned min, unsigned max)
{
	CTimer::SimpleusDelay (min);
}

static inline void msleep(unsigned ms)
{
	CTimer::SimpleMsDelay (ms);
}

#else

static inline void usleep_range(unsigned min, unsigned max)
{
	CScheduler::Get ()->usSleep (min);
}

static inline void msleep(unsigned ms)
{
	CScheduler::Get ()->MsSleep (ms);
}

#endif

static inline void udelay(unsigned us)
{
	CTimer::SimpleusDelay (us);
}

#endif
