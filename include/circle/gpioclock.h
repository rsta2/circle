//
// gpioclock.h
//
#ifndef _circle_gpioclock_h
#define _circle_gpioclock_h

#include <circle/types.h>

enum TGPIOClock
{
	GPIOClockCAM0 = 8,
	GPIOClockCAM1 = 9,
	GPIOClock0    = 14,			// on GPIO4 Alt0 or GPIO20 Alt5
	GPIOClock1    = 15,			// RPi 4: on GPIO5 Alt0 or GPIO21 Alt5
	GPIOClock2    = 16,			// on GPIO6 Alt0
	GPIOClockPCM  = 19,
	GPIOClockPWM  = 20
};

enum TGPIOClockSource
{						// RPi 1-3:		RPi 4:
	GPIOClockSourceOscillator = 1,		// 19.2 MHz		54 MHz
	GPIOClockSourcePLLC       = 5,		// 1000 MHz (varies)	1000 MHz (may vary)
	GPIOClockSourcePLLD       = 6,		// 500 MHz		750 MHz
	GPIOClockSourceHDMI       = 7,		// 216 MHz		unused
	GPIOClockSourceUnknown    = 16
};

class CGPIOClock
{
public:
	CGPIOClock (TGPIOClock Clock, TGPIOClockSource Source = GPIOClockSourceUnknown);
	~CGPIOClock (void);

						// refer to "BCM2835 ARM Peripherals" for that values:
	void Start (unsigned	nDivI,		// 1..4095, allowed minimum depends on MASH
		    unsigned	nDivF = 0,	// 0..4095
		    unsigned	nMASH = 0);	// 0..3

	// assigns clock source automatically
	// returns FALSE if requested rate cannot be generated
	boolean StartRate (unsigned nRateHZ);

	void Stop (void);
	
private:
	TGPIOClock m_Clock;
	TGPIOClockSource m_Source;
};

#endif
