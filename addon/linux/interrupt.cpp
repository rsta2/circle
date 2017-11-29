#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/bug.h>
#include <circle/interrupt.h>

static void irq_stub (void *param)
{
	struct irqaction *irqaction = reinterpret_cast<struct irqaction *> (param);

	irqreturn_t ret = (*irqaction->handler) (irqaction->irq, irqaction->dev_id);

	BUG_ON (ret != IRQ_HANDLED);
}

int __must_check devm_request_irq (struct device *dev, unsigned int irq, irq_handler_t handler,
				   unsigned long irqflags, const char *devname, void *dev_id)
{
	BUG_ON (irqflags != IRQF_IRQPOLL);

	dev->irqaction[0].irq = irq;
	dev->irqaction[0].handler = handler;
	dev->irqaction[0].flags = irqflags;
	dev->irqaction[0].dev_id = dev_id;

	CInterruptSystem *pInterruptSystem =
		reinterpret_cast<CInterruptSystem *> (dev->pInterruptSystem);

	pInterruptSystem->ConnectIRQ (irq, irq_stub, &dev->irqaction[0]);

	return 0;
}
