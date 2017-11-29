#include <linux/platform_device.h>

struct resource *platform_get_resource (struct platform_device *pdev, unsigned flags, unsigned index)
{
	for (unsigned i = 0; i < pdev->num_resources; i++)
	{
		if (   (pdev->resource[i].flags & flags)
		    && index-- == 0)
		{
			return &pdev->resource[i];
		}
	}

	return 0;
}

int platform_get_irq (struct platform_device *pdev, unsigned index)
{
	struct resource *res = platform_get_resource (pdev, IORESOURCE_IRQ, index);
	if (res == 0)
	{
		return -ENXIO;
	}

	return res->start;
}
