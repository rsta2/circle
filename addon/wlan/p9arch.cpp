#include "p9arch.h"
#include <circle/timer.h>
#include <circle/synchronize.h>
#include <circle/machineinfo.h>
#include <circle/interrupt.h>
#include <circle/dmachannel.h>
#include <circle/bcm2712.h>
#include <circle/memio.h>
#include <assert.h>

static struct machine_t machine;
struct machine_t *m = &machine;

#if RASPPI <= 4
static CDMAChannel *s_pDMA;
#endif

void gpiosel (unsigned pin, gpio_mode_t mode)
{
#if RASPPI <= 4
	uintptr nSelReg = ARM_GPIO_GPFSEL0 + (pin / 10) * 4;
	unsigned nShift = (pin % 10) * 3;

	PeripheralEntry ();

	u32 nValue = read32 (nSelReg);
	nValue &= ~(7 << nShift);
	nValue |= mode << nShift;
	write32 (nSelReg, nValue);

	PeripheralExit ();
#else
	uintptr nSelReg = ARM_PINCTRL1_BASE + (pin / 8) * 4;
	unsigned nShift = (pin % 8) * 4;

	u32 nValue = read32 (nSelReg);
	nValue &= ~(0xF << nShift);
	nValue |= mode << nShift;
	write32 (nSelReg, nValue);

	if (mode == Output)
	{
		assert (pin < 32);	// only bank 0 implemented
		write32 (ARM_GPIO1_IODIR0, read32 (ARM_GPIO1_IODIR0) & ~BIT (pin));
	}
#endif
}

static void gpiopull (unsigned pin, unsigned mode)
{
	PeripheralEntry ();

#if RASPPI <= 3
	u32 nRegOffset = (pin / 32) * 4;
	u32 nRegMask = 1 << (pin % 32);

	// See: https://forums.raspberrypi.com/viewtopic.php?t=163352#p1059178
	uintptr nClkReg = ARM_GPIO_GPPUDCLK0 + nRegOffset;

	write32 (ARM_GPIO_GPPUD, mode);
	CTimer::SimpleusDelay (5);		// 1us should be enough, but to be sure
	write32 (nClkReg, nRegMask);
	CTimer::SimpleusDelay (5);		// 1us should be enough, but to be sure
	write32 (ARM_GPIO_GPPUD, 0);
	write32 (nClkReg, 0);
#elif RASPPI == 4
	uintptr nModeReg = ARM_GPIO_GPPUPPDN0 + (pin / 16) * 4;
	unsigned nShift = (pin % 16) * 2;

	u32 nValue = read32 (nModeReg);
	nValue &= ~(3 << nShift);
	nValue |= mode << nShift;
	write32 (nModeReg, nValue);
#else
	// See: https://forums.raspberrypi.com/viewtopic.php?t=362326#p2174597
	u32 offset = pin + 112;			// TODO: for BCM2712 C0 only
	u32 pad_bit = (offset % 15) * 2;
	uintptr pad_base = ARM_PINCTRL1_BASE;
	pad_base += (uintptr) ((offset / 15) * 4);

	u32 val = read32 (pad_base);
	val &= ~(3 << pad_bit);
	val |= (mode << pad_bit);
	write32 (pad_base, val);
#endif

	PeripheralExit ();
}

void gpiopulloff (unsigned pin)
{
	gpiopull (pin, 0);
}

void gpiopullup (unsigned pin)
{
#if RASPPI != 4
	gpiopull (pin, 2);
#else
	gpiopull (pin, 1);
#endif
}

#if RASPPI >= 5

void gpioset (unsigned pin, unsigned val)
{
	assert (pin < 32);	// only bank 0 implemented

	if (val)
	{
		write32 (ARM_GPIO1_DATA0, read32 (ARM_GPIO1_DATA0) | BIT (pin));
	}
	else
	{
		write32 (ARM_GPIO1_DATA0, read32 (ARM_GPIO1_DATA0) & ~BIT (pin));
	}
}

#endif

unsigned getclkrate (unsigned clk)
{
#if RASPPI <= 4
	return CMachineInfo::Get ()->GetClockRate (clk);
#else
	return 54000000;
#endif
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

#if RASPPI <= 4

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

#endif

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
#if RASPPI <= 4
	s_pDMA = new CDMAChannel (DMA_CHANNEL_NORMAL);
	assert (s_pDMA != 0);
#endif

	CTimer::Get ()->RegisterPeriodicHandler (PeriodicHandler);
}
