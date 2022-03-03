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

/// \note This driver supports an interrupt mode and a polling mode.

class CKY040	/// Driver for KY-040 rotary encoder module
{
public:
	enum TEvent
	{
		EventClockwise,
		EventCounterclockwise,

		EventSwitchDown,
		EventSwitchUp,
		EventSwitchClick,
		EventSwitchDoubleClick,
		EventSwitchTripleClick,
		EventSwitchHold,		///< generated each second

		EventUnknown
	};

	typedef void TEventHandler (TEvent Event, void *pParam);

public:
	/// \param nCLKPin GPIO pin number of clock pin (encoder pin A)
	/// \param nDTPin GPIO pin number of data pin (encoder pin B)
	/// \param nSWPin GPIO pin number of switch pin
	/// \param pGPIOManager Pointer to GPIO manager object (0 enables polling mode)
	CKY040 (unsigned nCLKPin, unsigned nDTPin, unsigned nSWPin, CGPIOManager *pGPIOManager = 0);

	~CKY040 (void);

	/// \brief Operation successful?
	boolean Initialize (void);

	/// \brief Register a handler, to be called on an event from the encoder
	/// \param pHandler Pointer to the handler
	/// \param pParam   Optional user parameter, handed over to the handler
	void RegisterEventHandler (TEventHandler *pHandler, void *pParam = 0);

	/// \return Number of seconds, the switch is hold down
	/// \note Only valid, when EventSwitchHold has been received.
	unsigned GetHoldSeconds (void) const;

	/// \brief Has to be called very frequently in polling mode
	void Update (void);

private:
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

	enum TSwitchState
	{
		SwitchStateStart,
		SwitchStateDown,
		SwitchStateClick,
		SwitchStateDown2,
		SwitchStateClick2,
		SwitchStateDown3,
		SwitchStateClick3,
		SwitchStateHold,
		SwitchStateInvalid,
		SwitchStateUnknown
	};

	enum TSwitchEvent
	{
		SwitchEventDown,
		SwitchEventUp,
		SwitchEventTick,
		SwitchEventUnknown
	};

private:
	void HandleSwitchEvent (TSwitchEvent SwitchEvent);

	static void EncoderInterruptHandler (void *pParam);
	static void SwitchInterruptHandler (void *pParam);

	static void SwitchDebounceHandler (TKernelTimerHandle hTimer, void *pParam, void *pContext);
	static void SwitchTickHandler (TKernelTimerHandle hTimer, void *pParam, void *pContext);

private:
	CGPIOPin m_CLKPin;
	CGPIOPin m_DTPin;
	CGPIOPin m_SWPin;

	boolean m_bPollingMode;
	boolean m_bInterruptConnected;

	TEventHandler *m_pEventHandler;
	void *m_pEventParam;

	// encoder
	TState m_State;

	static TState s_NextState[StateUnknown][2][2];
	static TEvent s_Output[StateUnknown][2][2];

	// switch low level
	TKernelTimerHandle m_hDebounceTimer;
	TKernelTimerHandle m_hTickTimer;

	unsigned m_nLastSWLevel;
	boolean m_bDebounceActive;
	unsigned m_nDebounceLastTicks;

	// switch higher level
	TSwitchState m_SwitchState;
	unsigned m_nSwitchLastTicks;
	unsigned m_nHoldCounter;

	static TSwitchState s_NextSwitchState[SwitchStateUnknown][SwitchEventUnknown];
	static TEvent s_SwitchOutput[SwitchStateUnknown][SwitchEventUnknown];
};

#endif
