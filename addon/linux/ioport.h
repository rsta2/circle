#ifndef _linux_ioport_h
#define _linux_ioport_h

#include <linux/types.h>

struct resource
{
	u32	 start;
	u32	 end;

	unsigned flags;
#define IORESOURCE_MEM		0x200
#define IORESOURCE_IRQ		0x400
#define IORESOURCE_DMA		0x800	// internally used for coherent region
};

#endif
