//
// pwmoutput-rp1.h
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
#ifndef _circle_pwmoutput_rp1_h
#define _circle_pwmoutput_rp1_h

#ifndef _circle_pwmoutput_h
	#error Do not include this header file directly!
#endif

#include <circle/gpioclock.h>
#include <circle/types.h>

class CPWMOutput
{
public:
	CPWMOutput (unsigned	     nClockRateHz,
		    unsigned	     nRange,		// Range (see "RP1 Peripherals")
		    boolean	     bMSMode,		// Trailing edge mode (see "RP1 Peripherals")
		    boolean	     bInvert = FALSE,	// Invert output?
		    unsigned	     nDevice = 0);	// PWM0 (default) or PWM1

	CPWMOutput (TGPIOClockSource Source,		// normally GPIOClockSourceXOscillator
		    unsigned	     nDivider,		// 1..65535
		    unsigned	     nRange,		// Range (see "RP1 Peripherals")
		    boolean	     bMSMode,		// Trailing edge mode (see "RP1 Peripherals")
		    boolean	     bInvert = FALSE,	// Invert output?
		    unsigned	     nDevice = 0);	// PWM0 (default) or PWM1

	~CPWMOutput (void);

	boolean Start (void);

	void Stop (void);

#define PWM_CHANNEL1	1
#define PWM_CHANNEL2	2
#define PWM_CHANNEL3	3
#define PWM_CHANNEL4	4
	void Write (unsigned nChannel, unsigned nValue);	// nValue: 0..Range

private:
	CGPIOClock m_Clock;
	unsigned   m_nClockRateHz;
	unsigned   m_nDivider;
	unsigned   m_nRange;
	boolean    m_bMSMode;
	boolean    m_bInvert;
	uintptr	   m_ulBase;
	boolean    m_bActive;
};

#endif
