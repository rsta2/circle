//
// gpioclock.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2024  R. Stange <rsta2@o2online.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef _circle_gpioclock_h
#define _circle_gpioclock_h

#if RASPPI >= 5
	#include <circle/gpioclock-rp1.h>
#else

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
	boolean Start (unsigned	nDivI,		// 1..4095, allowed minimum depends on MASH
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

#endif
