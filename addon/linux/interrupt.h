#ifndef _linux_interrupt_h
#define _linux_interrupt_h

#include <linux/compiler.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum irqreturn
{
	IRQ_NONE	= (0 << 0),	// interrupt was not from this device or was not handled
	IRQ_HANDLED	= (1 << 0),	// interrupt was handled by this device
	IRQ_WAKE_THREAD	= (1 << 1)	// handler requests to wake the handler thread
}
irqreturn_t;

typedef irqreturn_t (*irq_handler_t) (int irq, void *dev_id);

struct irqaction
{
	irq_handler_t	 handler;
	void		*dev_id;
	unsigned int	 irq;
	unsigned int	 flags;
#define IRQF_IRQPOLL 0x1000
};

struct device;

int __must_check devm_request_irq (struct device *dev, unsigned int irq, irq_handler_t handler,
				   unsigned long irqflags, const char *devname, void *dev_id);

#ifdef __cplusplus
}
#endif

#endif
