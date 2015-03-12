//
// pwmoutput.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2015  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_pwmoutput_h
#define _circle_pwmoutput_h

#include <circle/gpioclock.h>
#include <circle/spinlock.h>
#include <circle/types.h>

class CPWMOutput
{
public:
	CPWMOutput (TGPIOClockSource Source,		// see gpioclock.h
		    unsigned	     nDivider,		// 1..4095
		    unsigned	     nRange,		// Range (see "BCM2835 ARM Peripherals")
		    boolean	     bMSMode);		// M/S mode (see "BCM2835 ARM Peripherals")
	~CPWMOutput (void);

	void Start (void);
	void Stop (void);

#define PWM_CHANNEL1	1
#define PWM_CHANNEL2	2
	void Write (unsigned nChannel, unsigned nValue);	// nValue: 0..Range

private:
	CGPIOClock m_Clock;
	unsigned   m_nDivider;
	unsigned   m_nRange;
	boolean    m_bMSMode;
	boolean    m_bActive;
	CSpinLock  m_SpinLock;
};

#endif
