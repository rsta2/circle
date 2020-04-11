#include "p9arch.h"
#include <circle/timer.h>
#include <circle/synchronize.h>
#include <circle/machineinfo.h>
#include <circle/interrupt.h>
#include <circle/dmachannel.h>
#include <circle/memio.h>
#include <assert.h>

static struct machine_t machine;
struct machine_t *m = &machine;

static CDMAChannel *s_pDMA;

void gpiosel (unsigned pin, gpio_mode_t mode)
{
	uintptr nSelReg = ARM_GPIO_GPFSEL0 + (pin / 10) * 4;
	unsigned nShift = (pin % 10) * 3;

	PeripheralEntry ();

	u32 nValue = read32 (nSelReg);
	nValue &= ~(7 << nShift);
	nValue |= mode << nShift;
	write32 (nSelReg, nValue);

	PeripheralExit ();
}

static void gpiopull (unsigned pin, unsigned mode)
{
	PeripheralEntry ();

#if RASPPI <= 3
	u32 nRegOffset = (pin / 32) * 4;
	u32 nRegMask = 1 << (pin % 32);

	// See: http://www.raspberrypi.org/forums/viewtopic.php?t=163352&p=1059178#p1059178
	uintptr nClkReg = ARM_GPIO_GPPUDCLK0 + nRegOffset;

	write32 (ARM_GPIO_GPPUD, mode);
	CTimer::SimpleusDelay (5);		// 1us should be enough, but to be sure
	write32 (nClkReg, nRegMask);
	CTimer::SimpleusDelay (5);		// 1us should be enough, but to be sure
	write32 (ARM_GPIO_GPPUD, 0);
	write32 (nClkReg, 0);
#else
	uintptr nModeReg = ARM_GPIO_GPPUPPDN0 + (pin / 16) * 4;
	unsigned nShift = (pin % 16) * 2;

	u32 nValue = read32 (nModeReg);
	nValue &= ~(3 << nShift);
	nValue |= mode << nShift;
	write32 (nModeReg, nValue);
#endif

	PeripheralExit ();
}

void gpiopulloff (unsigned pin)
{
	gpiopull (pin, 0);
}

void gpiopullup (unsigned pin)
{
#if RASPPI <= 3
	gpiopull (pin, 2);
#else
	gpiopull (pin, 1);
#endif
}

unsigned getclkrate (unsigned clk)
{
	return CMachineInfo::Get ()->GetClockRate (clk);
}

void delay (unsigned msecs)
{
	CTimer::Get ()->MsDelay (msecs);
}

void microdelay (unsigned usecs)
{
	CTimer::Get ()->usDelay (usecs);
}

void coherence (void)
{
	DataSyncBarrier ();
}

static irqhandler_t *s_pIRQHandler;

static void IRQStub (void *context)
{
	(*s_pIRQHandler) (0, context);
}

void intrenable (unsigned irq, irqhandler_t *handler, void *context, unsigned, const char *name)
{
	s_pIRQHandler = handler;
	CInterruptSystem::Get ()->ConnectIRQ (irq, IRQStub, context);
}

void dmastart (unsigned chan, unsigned dev, unsigned dir, void *from, void *to, size_t len)
{
	if (dir == DmaD2M)
	{
		s_pDMA->SetupIORead (to, (u32) (uintptr) from, len, (TDREQ) dev);
	}
	else
	{
		s_pDMA->SetupIOWrite ((u32) (uintptr) to, from, len, (TDREQ) dev);
	}

	s_pDMA->Start ();
}

int dmawait (unsigned chan)
{
	if (!s_pDMA->Wait ())
	{
		return -1;
	}

	return 0;
}

void cachedinvse (void *buf, size_t len)
{
	CleanAndInvalidateDataCacheRange ((uintptr) buf, len);
}

static void PeriodicHandler (void)
{
	m->ticks++;
}

void p9arch_init(void)
{
	s_pDMA = new CDMAChannel (DMA_CHANNEL_NORMAL);
	assert (s_pDMA != 0);

	CTimer::Get ()->RegisterPeriodicHandler (PeriodicHandler);
}
