//
/// \file pwmoutput.h
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
#ifndef _circle_pwmoutput_h
#define _circle_pwmoutput_h

#if RASPPI >= 5
	#include <circle/pwmoutput-rp1.h>
#else

#include <circle/gpioclock.h>
#include <circle/spinlock.h>
#include <circle/types.h>

#define PWM_CHANNEL1	1	///< First PWM channel
#define PWM_CHANNEL2	2	///< Second PWM channel

class CPWMOutput
{
public:
	/// \param nClockRateHz Frequency of the PWM clock in Hz
	/// \param nRange Range parameter (see "BCM2835 ARM Peripherals")
	/// \param bMSMode Enable M/S mode? (see "BCM2835 ARM Peripherals")
	/// \param bInvert Invert PWM output?
	/// \param nDevice Device number (normally 0)
	CPWMOutput (unsigned	     nClockRateHz,
		    unsigned	     nRange,
		    boolean	     bMSMode,
		    boolean	     bInvert = FALSE,
		    unsigned	     nDevice = 0);

	/// \param Source Clock source to be used (see gpioclock.h)
	/// \param nDivider Clock divider (1..4095)
	/// \param nRange Range parameter (see "BCM2835 ARM Peripherals")
	/// \param bMSMode Enable M/S mode? (see "BCM2835 ARM Peripherals")
	/// \param bInvert Invert PWM output?
	/// \param nDevice Device number (normally 0)
	CPWMOutput (TGPIOClockSource Source,
		    unsigned	     nDivider,
		    unsigned	     nRange,
		    boolean	     bMSMode,
		    boolean	     bInvert = FALSE,
		    unsigned	     nDevice = 0);

	~CPWMOutput (void);

	/// \brief Start PWM Operation
	/// \return Operation successful?
	boolean Start (void);

	/// \brief Stop PWM Operation
	void Stop (void);

	/// \param nChannel PWM_CHANNEL1 or PWM_CHANNEL2
	/// \param nValue Value to be written (0..Range)
	void Write (unsigned nChannel, unsigned nValue);

private:
	CGPIOClock m_Clock;
	unsigned   m_nClockRateHz;
	unsigned   m_nDivider;
	unsigned   m_nRange;
	boolean    m_bMSMode;
	boolean    m_bInvert;
	boolean    m_bActive;
	CSpinLock  m_SpinLock;
};

#endif

#endif
