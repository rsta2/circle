#ifndef _linux_platform_device_h
#define _linux_platform_device_h

#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct platform_device
{
	struct device	dev;
	u32		num_resources;
#define PLATFORM_DEVICE_MAX_RESOURCES	8
	struct resource	resource[PLATFORM_DEVICE_MAX_RESOURCES];
};

static inline void platform_set_drvdata (struct platform_device *pdev, void *data)
{
	pdev->dev.driver_data = data;
}

static inline void *platform_get_drvdata (const struct platform_device *pdev)
{
	return pdev->dev.driver_data;
}

struct resource *platform_get_resource (struct platform_device *pdev, unsigned flags, unsigned index);

int platform_get_irq (struct platform_device *pdev, unsigned index);

#ifdef __cplusplus
}
#endif

#endif
