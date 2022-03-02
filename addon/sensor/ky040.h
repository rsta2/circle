//
// ky040.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2022  R. Stange <rsta2@o2online.de>
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
#ifndef _sensor_ky040_h
#define _sensor_ky040_h

#include <circle/gpiomanager.h>
#include <circle/gpiopin.h>
#include <circle/timer.h>
#include <circle/types.h>

class CKY040	/// Driver for KY-040 rotary encoder module
{
public:
	enum TEvent
	{
		EventClockwise,
		EventCounterclockwise,
		EventSwitchDown,
		EventSwitchUp,
		EventUnknown
	};

	typedef void TEventHandler (TEvent Event, void *pParam);

public:
	/// \param pGPIOManager Pointer to GPIO manager object
	/// \param nCLKPin GPIO pin number of clock pin (encoder pin A)
	/// \param nDTPin GPIO pin number of data pin (encoder pin B)
	/// \param nSWPin GPIO pin number of switch pin
	CKY040 (CGPIOManager *pGPIOManager, unsigned nCLKPin, unsigned nDTPin, unsigned nSWPin);

	~CKY040 (void);

	/// \brief Register a handler, to be called on an event from the encoder
	/// \param pHandler Pointer to the handler
	/// \param pParam   Optional user parameter
	void RegisterEventHandler (TEventHandler *pHandler, void *pParam = 0);

private:
	static void EncoderInterruptHandler (void *pParam);

	static void SwitchInterruptHandler (void *pParam);
	static void SwitchTimerHandler (TKernelTimerHandle hTimer, void *pParam, void *pContext);

	enum TState
	{
		StateStart,
		StateCWStart,
		StateCWBothLow,
		StateCWFirstHigh,
		StateCCWStart,
		StateCCWBothLow,
		StateCCWFirstHigh,
		StateInvalid,
		StateUnknown
	};

private:
	CGPIOPin m_CLKPin;
	CGPIOPin m_DTPin;
	CGPIOPin m_SWPin;

	TEventHandler *m_pEventHandler;
	void *m_pEventParam;

	TState m_State;

	TKernelTimerHandle m_hDebounceTimer;

	static TState s_NextState[StateUnknown][2][2];
	static TEvent s_Output[StateUnknown][2][2];
};

#endif
