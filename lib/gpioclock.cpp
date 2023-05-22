//
// gpioclock.cpp
//
// This was mainly posted by joan@raspi-forum:
// 	10MHzClock.c
// 	2014-10-24
// 	Public Domain
//
#include <circle/gpioclock.h>
#include <circle/bcm2835.h>
#include <circle/timer.h>
#include <circle/memio.h>
#include <circle/synchronize.h>
#include <circle/machineinfo.h>
#include <assert.h>

#define CLK_CTL_MASH(x)		((x) << 9)
#define CLK_CTL_BUSY		(1 << 7)
#define CLK_CTL_KILL		(1 << 5)
#define CLK_CTL_ENAB		(1 << 4)
#define CLK_CTL_SRC(x)		((x) << 0)

#define CLK_DIV_DIVI(x)		((x) << 12)
#define CLK_DIV_DIVF(x)		((x) << 0)

CGPIOClock::CGPIOClock (TGPIOClock Clock, TGPIOClockSource Source)
:	m_Clock (Clock),
	m_Source (Source)
{
	assert (m_Source <= GPIOClockSourceUnknown);
}

CGPIOClock::~CGPIOClock (void)
{
	Stop ();
}

void CGPIOClock::Start (unsigned nDivI, unsigned nDivF, unsigned nMASH)
{
#ifndef NDEBUG
	static unsigned MinDivI[] = {1, 2, 3, 5};

	assert (nMASH <= 3);
	assert (MinDivI[nMASH] <= nDivI && nDivI <= 4095);
	assert (nDivF <= 4095);
#endif

	unsigned nCtlReg = ARM_CM_BASE + (m_Clock * 8);
	unsigned nDivReg  = nCtlReg + 4;

	Stop ();

	PeripheralEntry ();

	write32 (nDivReg, ARM_CM_PASSWD | CLK_DIV_DIVI (nDivI) | CLK_DIV_DIVF (nDivF));

	CTimer::SimpleusDelay (10);

	assert (m_Source < GPIOClockSourceUnknown);
	write32 (nCtlReg, ARM_CM_PASSWD | CLK_CTL_MASH (nMASH) | CLK_CTL_SRC (m_Source));

	CTimer::SimpleusDelay (10);

	write32 (nCtlReg, read32 (nCtlReg) | ARM_CM_PASSWD | CLK_CTL_ENAB);

	PeripheralExit ();
}

boolean CGPIOClock::StartRate (unsigned nRateHZ)
{
	assert (nRateHZ > 0);

	for (unsigned nSourceId = 0; nSourceId <= GPIO_CLOCK_SOURCE_ID_MAX; nSourceId++)
	{
		unsigned nSourceRate = CMachineInfo::Get ()->GetGPIOClockSourceRate (nSourceId);
		if (nSourceRate == GPIO_CLOCK_SOURCE_UNUSED)
		{
			continue;
		}

		unsigned nDivI = nSourceRate / nRateHZ;
		if (   1 <= nDivI && nDivI <= 4095
		    && nRateHZ * nDivI == nSourceRate)
		{
			m_Source = (TGPIOClockSource) nSourceId;

			Start (nDivI, 0, 0);

			return TRUE;
		}
	}

	return FALSE;
}

void CGPIOClock::Stop (void)
{
	unsigned nCtlReg = ARM_CM_GP0CTL + (m_Clock * 8);

	PeripheralEntry ();

	write32 (nCtlReg, ARM_CM_PASSWD | CLK_CTL_KILL);
	while (read32 (nCtlReg) & CLK_CTL_BUSY)
	{
		// wait for clock to stop
	}

	PeripheralExit ();
}
