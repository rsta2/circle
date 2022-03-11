//
// cputhrottle.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2022  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_cputhrottle_h
#define _circle_cputhrottle_h

#include <circle/gpiopin.h>
#include <circle/macros.h>
#include <circle/types.h>

enum TCPUSpeed
{
	CPUSpeedLow,
	CPUSpeedMaximum,
	CPUSpeedUnknown
};

enum TSystemThrottledState
{
	SystemStateNothingOccurred		= 0,

	SystemStateUnderVoltageOccurred		= BIT (16),
	SystemStateFrequencyCappingOccurred	= BIT (17),
	SystemStateThrottlingOccurred		= BIT (18),
	SystemStateSoftTempLimitOccurred	= BIT (19)
};

typedef void TSystemThrottledHandler (TSystemThrottledState CurrentState, void *pParam);

/// \warning You have to repeatedly call SetOnTemperature() or Update() if you use this class!\n
///	     See the description of SetOnTemperature() for details!\n
///	     IF YOU ARE NOT SURE ABOUT HOW TO MANAGE THIS, DO NOT USE THIS CLASS!

/// \warning CCPUThrottle cannot be used together with code doing I2C or SPI transfers.\n
///	     Because clock rate changes to the CPU clock may also effect the CORE clock,\n
///	     this could result in a changing transfer speed.

/// \note If the cmdline.txt option "gpiofanpin=" is used, this class controls a GPIO fan\n
///	  not the CPU clock rate.

class CCPUThrottle	/// Manages CPU clock rate depending on app/user requirements and SoC temperature
{
public:
	/// \param InitialSpeed CPU speed to be set initially\n
	/// CPUSpeedUnknown: use the cmdline.txt parameter "fast=true" or CPUSpeedLow (if not set)\n
	/// Otherwise: the selected value
	CCPUThrottle (TCPUSpeed InitialSpeed = CPUSpeedUnknown);
	~CCPUThrottle (void);

	/// \return Is CPU clock rate change supported?\n
	/// Other Methods can be called in any case, but may be nop's\n
	/// or return invalid values if IsDynamic() returns FALSE.
	boolean IsDynamic (void) const;

	/// \return Current CPU clock rate in Hz, 0 on failure
	unsigned GetClockRate (void) const;
	/// \return Minimum CPU clock rate in Hz
	unsigned GetMinClockRate (void) const;
	/// \return Maximum CPU clock rate in Hz
	unsigned GetMaxClockRate (void) const;

	/// \return Current SoC temperature in degrees Celsius, 0 on failure
	unsigned GetTemperature (void) const;
	/// \return Maximum SoC temperature in degrees Celsius
	unsigned GetMaxTemperature (void) const;

	/// \brief Sets the CPU speed
	/// \param Speed Speed to be set (overwrites intial value)
	/// \param bWait Wait for new clock rate to settle?
	/// \return Previous setting or CPUSpeedUnknown on error
	TCPUSpeed SetSpeed (TCPUSpeed Speed, boolean bWait = TRUE);

	/// \brief Sets the CPU speed depending on current SoC temperature.\n
	/// Call this repeatedly all 2 to 5 seconds to hold temperature down!\n
	/// Throttles the CPU down when the SoC temperature reaches 60 degrees Celsius\n
	/// (or the value set using the cmdline.txt parameter "socmaxtemp").
	/// \return Operation successful?
	boolean SetOnTemperature (void);

	/// \brief Same function as SetOnTemperature(), but can be called\n
	/// as often as you want without checking the calling interval.\n
	/// Additionally checks for system throttled conditions, if a system\n
	/// throttled handler is registered.
	/// \return Operation successful?
	boolean Update (void);

	/// \brief Register a callback function, which is called from Update(),\n
	/// when a system throttled condition occurs, which is given in StateMask.
	/// \param StateMask TSystemThrottledState values to be watched (or'ed together)
	/// \param pHandler Callback function to be called
	/// \param pParam User parameter to be handed over to the callback function
	void RegisterSystemThrottledHandler (unsigned StateMask,
					     TSystemThrottledHandler *pHandler, void *pParam = 0);

	/// \brief Dump some information on the current CPU status
	/// \param bAll Dump all information (only current clock rate and temperature otherwise)
	void DumpStatus (boolean bAll = TRUE);

	/// \return Pointer to the only CCPUThrottle object in the system
	static CCPUThrottle *Get (void);

private:
	boolean SetSpeedInternal (TCPUSpeed Speed, boolean bWait);

	boolean CheckThrottledState (void);

	void SetToSetDelay (void);

	static unsigned GetClockRate (unsigned nTagId);		// returns 0 on failure
	static unsigned GetTemperature (unsigned nTagId);	// returns 0 on failure
	static boolean SetClockRate (unsigned nRate, boolean bSkipTurbo);

private:
	boolean  m_bDynamic;
	unsigned m_nMinClockRate;
	unsigned m_nMaxClockRate;
	unsigned m_nMaxTemperature;
	unsigned m_nEnforcedTemperature;

	TCPUSpeed m_SpeedSet;
	unsigned  m_nTicksLastSet;
	unsigned  m_nTicksLastUpdate;

	TSystemThrottledState m_ThrottledStateMask;
	TSystemThrottledState m_LastThrottledState;
	TSystemThrottledHandler *m_pThrottledHandler;
	void *m_pThrottledParam;

	boolean m_bFanConnected;
	CGPIOPin m_FanPin;

	static CCPUThrottle *s_pThis;
};

#endif
