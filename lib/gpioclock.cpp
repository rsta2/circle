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
	assert (m_Clock <= GPIOClockPWM);
	assert (m_Source <= GPIOClockSourceHDMI);
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

	unsigned nCtlReg = ARM_CM_GP0CTL + (m_Clock * 8);
	unsigned nDivReg  = ARM_CM_GP0DIV + (m_Clock * 8);

	Stop ();

	write32 (nDivReg, ARM_CM_PASSWD | CLK_DIV_DIVI (nDivI) | CLK_DIV_DIVF (nDivF));

	CTimer::SimpleusDelay (10);

	write32 (nCtlReg, ARM_CM_PASSWD | CLK_CTL_MASH (nMASH) | CLK_CTL_SRC (m_Source));

	CTimer::SimpleusDelay (10);

	write32 (nCtlReg, read32 (nCtlReg) | ARM_CM_PASSWD | CLK_CTL_ENAB);

	PeripheralExit ();
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
