#ifndef _rp1dsi_linuxcompat_h
#define _rp1dsi_linuxcompat_h

#include <circle/sched/scheduler.h>
#include <circle/sysconfig.h>
#include <circle/timer.h>
#include <circle/types.h>

#ifndef EINVAL
	#define EINVAL	1
#endif

#ifndef EIO
	#define EIO	2
#endif

#ifndef ENOSYS
	#define ENOSYS	3
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
