//
// softpwmoutput.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018  R. Stange <rsta2@o2online.de>
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
#ifndef _softpwmoutput_h
#define _softpwmoutput_h

#include <circle/gpiopin.h>
#include <circle/types.h>

class CSoftPWMOutput		/// Software PWM output via GPIO pin
{
public:
	/// \param nGPIOPin Number of the used GPIO pin
	/// \param nRange PWM range (>= 1)
	CSoftPWMOutput (unsigned nGPIOPin, unsigned nRange);

	~CSoftPWMOutput (void);

	/// \brief Start PWM output
	void Start (void);
	/// \brief Stop PWM output
	void Stop (void);

	/// \param nValue PWM value to be written (<= Range)
	void Write (unsigned nValue);

public:
	/// \brief Must be called continuously with PWM clock frequency
	/// \note Normally called from CUserTimer interrupt handler
	void ClockTick (void);

private:
	CGPIOPin m_GPIOPin;
	unsigned m_nRange;

	volatile boolean  m_bRunning;
	volatile unsigned m_nValue;

	unsigned m_nCurrentValue;	// copy of m_nValue for the current cycle
	unsigned m_nLevel;		// current output level (LOW or HIGH)
	unsigned m_nCount;		// current position inside m_nRange
};

#endif
