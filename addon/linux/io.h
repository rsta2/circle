#ifndef _linux_io_h
#define _linux_io_h

#include <linux/types.h>
#include <circle/memio.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline u32 readl (const volatile void __iomem *addr)
{
	return read32 ((uintptr) addr);
}


static inline void writel (u32 val, volatile void __iomem *addr)
{
	write32 ((uintptr) addr, val);
}

#ifdef __cplusplus
}
#endif

#endif
