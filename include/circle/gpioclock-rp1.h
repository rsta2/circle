//
// gpioclock-rp1.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2024  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_gpioclock_rp1_h
#define _circle_gpioclock_rp1_h

#ifndef _circle_gpioclock_h
	#error Do not include this header file directly!
#endif

#include <circle/types.h>

enum TGPIOClock
{
	GPIOClockPWM0		= 6,
	GPIOClockPWM1		= 7,
	GPIOClockAudioIn	= 8,
	GPIOClockAudioOut	= 9,
	GPIOClockI2S		= 10,

	GPIOClock0		= 22,		// on GPIO4 Alt0 or GPIO20 Alt3
	GPIOClock1		= 23,		// on GPIO5 Alt0, GPIO18 Alt8 or GPIO21 Alt3
	GPIOClock2		= 24,		// on GPIO6 Alt0

	GPIOClockUnknown	= 28
};

enum TGPIOClockSource
{
	GPIOClockSourceNone = 0,

	GPIOClockSourceXOscillator,		// 50 MHz

	GPIOClockSourcePLLSys,			// 200 MHz
	GPIOClockSourcePLLSysSec,		// 125 MHz
	GPIOClockSourcePLLSysPriPh,		// 100 MHz

	GPIOClockSourcePLLAudio,		// 61440 KHz
	GPIOClockSourcePLLAudioSec,		// 192 MHz
	GPIOClockSourcePLLAudioPriPh,		// 30720 KHz

	GPIOClockSourceClkSys,			// 200 MHz

	GPIOClockSourceUnknown
};

class CGPIOClock
{
public:
	CGPIOClock (TGPIOClock Clock, TGPIOClockSource Source = GPIOClockSourceUnknown);
	~CGPIOClock (void);

	void Start (unsigned nDivI,		// 1..65535, Audio/I2S: 1..255
		    unsigned nDivF = 0);	// 0..65535

	// assigns clock source automatically
	// returns FALSE if requested rate cannot be generated
	boolean StartRate (unsigned nRateHZ);

	void Stop (void);

#ifndef NDEBUG
	static void DumpStatus (boolean bEnabledOnly = TRUE);
#endif
	
private:
	TGPIOClock m_Clock;
	TGPIOClockSource m_Source;

	unsigned m_nDivIntMax;
	unsigned m_nFreqMax;
	unsigned m_nAuxSrc;

	static const unsigned MaxParents = 16;
	static const u8 s_ParentAux[GPIOClockUnknown][MaxParents];
};

#endif
