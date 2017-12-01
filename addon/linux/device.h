#ifndef _linux_device_h
#define _linux_device_h

#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct device
{
	void *driver_data;

	void *pMemorySystem;		// opaque pointer
	void *pInterruptSystem;		// opaque pointer

	struct resource dma_mem;	// internally used
	struct irqaction irqaction[1];	// internally used
};

void __iomem *devm_ioremap_resource (struct device *dev, struct resource *res);

void dev_err (const struct device *dev, const char *fmt, ...);
void dev_warn (const struct device *dev, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
