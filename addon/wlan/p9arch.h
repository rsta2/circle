#ifndef _p9arch_h
#define _p9arch_h

#include <circle/bcm2835.h>
#include <circle/bcm2835int.h>
#include <circle/memorymap.h>
#include "p9util.h"

#ifdef __cplusplus
extern "C" {
#endif

#if RASPPI <= 4
#define VIRTIO		ARM_IO_BASE
#else
#define VIRTIO		MEM_IOMEM_AXI_START
#endif

enum gpio_mode_t
{
#if RASPPI <= 4
	Alt0 = 4,
	Alt1 = 5,
	Alt2 = 6,
	Alt3 = 7,
	Alt4 = 3,
	Alt5 = 2
#else
	Output,
	Func1,
	Func2,
	Func3,
	Func4,
	Func5,
	Func6,
	Func7,
	Func8
#endif
};

void gpiosel (unsigned pin, enum gpio_mode_t mode);

void gpiopulloff (unsigned pin);
void gpiopullup (unsigned pin);

#if RASPPI >= 5
void gpioset (unsigned pin, unsigned val);

unsigned get_soc_stepping (void);
#define SOC_STEPPING_D0	0x30
#endif

unsigned getclkrate (unsigned clk);
#define ClkEmmc		1

#define delay		__p9delay
#define microdelay	__p9microdelay
void delay (unsigned msecs);
void microdelay (unsigned usecs);

void coherence (void);

typedef u32	Ureg;
typedef void irqhandler_t (Ureg *, void *context);
void intrenable (unsigned irq, irqhandler_t *handler, void *context, unsigned, const char *name);
#if RASPPI <= 4
#define IRQmmc		ARM_IRQ_ARASANSDIO
#else
#define IRQmmc		ARM_IRQ_SDIO2
#endif

#if RASPPI <= 4
void dmastart (unsigned chan, unsigned dev, unsigned dir, void *from, void *to, size_t len);
int dmawait (unsigned chan);
#define DmaChanEmmc	0		// unused
#define DmaDevEmmc	11
#define DmaM2D		0
#define DmaD2M		1
#endif

void cachedinvse (void *buf, size_t len);

#define m	__p9m
extern struct machine_t
{
	volatile unsigned ticks;
#define HZ		100
}
*m;

void p9arch_init (void);

#ifdef __cplusplus
}
#endif

#endif
