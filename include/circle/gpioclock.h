//
// gpioclock.h
//
#ifndef _circle_gpioclock_h
#define _circle_gpioclock_h

enum TGPIOClock
{
	GPIOClock0   = 0,			// on GPIO4 Alt0 or GPIO20 Alt5
	GPIOClock2   = 2,			// on GPIO6 Alt0
	GPIOClockPCM = 5,
	GPIOClockPWM = 6
};

enum TGPIOClockSource
{
	GPIOClockSourceOscillator = 1,		// 19.2 MHz
	GPIOClockSourcePLLC       = 5,		// 1000 MHz (changes with overclock settings)
	GPIOClockSourcePLLD       = 6,		// 500 MHz
	GPIOClockSourceHDMI       = 7		// 216 MHz
};

class CGPIOClock
{
public:
	CGPIOClock (TGPIOClock Clock, TGPIOClockSource Source);
	~CGPIOClock (void);

						// refer to "BCM2835 ARM Peripherals" for that values:
	void Start (unsigned	nDivI,		// 1..4095, allowed minimum depends on MASH
		    unsigned	nDivF = 0,	// 0..4095
		    unsigned	nMASH = 0);	// 0..3

	void Stop (void);
	
private:
	TGPIOClock m_Clock;
	TGPIOClockSource m_Source;
};

#endif
