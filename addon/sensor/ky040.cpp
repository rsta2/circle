//
// ky040.cpp
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
#include "ky040.h"
#include <assert.h>

static const unsigned SwitchDebounceDelayMillis	= 50;
static const unsigned SwitchTickDelayMillis = 500;

CKY040::TState CKY040::s_NextState[StateUnknown][2][2] =
{
	// {{CLK=0/DT=0, CLK=0/DT=1}, {CLK=1/DT=0, CLK=1/DT=1}}

	{{StateInvalid,    StateCWStart},      {StateCCWStart,    StateStart}},   // StateStart

	{{StateCWBothLow,  StateCWStart},      {StateInvalid,     StateStart}},   // StateCWStart
	{{StateCWBothLow,  StateInvalid},      {StateCWFirstHigh, StateInvalid}}, // StateCWBothLow
	{{StateInvalid,    StateInvalid},      {StateCWFirstHigh, StateStart}},   // StateCWFirstHigh

	{{StateCCWBothLow, StateInvalid},      {StateCCWStart,    StateStart}},   // StateCCWStart
	{{StateCCWBothLow, StateCCWFirstHigh}, {StateInvalid,     StateInvalid}}, // StateCCWBothLow
	{{StateInvalid,    StateCCWFirstHigh}, {StateInvalid,     StateStart}},   // StateCCWFirstHigh

	{{StateInvalid,    StateInvalid},      {StateInvalid,     StateStart}}    // StateInvalid
};

CKY040::TEvent CKY040::s_Output[StateUnknown][2][2] =
{
	// {{CLK=0/DT=0, CLK=0/DT=1}, {CLK=1/DT=0, CLK=1/DT=1}}

	{{EventUnknown, EventUnknown}, {EventUnknown, EventUnknown}},          // StateStart

	{{EventUnknown, EventUnknown}, {EventUnknown, EventUnknown}},          // StateCWStart
	{{EventUnknown, EventUnknown}, {EventUnknown, EventUnknown}},          // StateCWBothLow
	{{EventUnknown, EventUnknown}, {EventUnknown, EventClockwise}},        // StateCWFirstHigh

	{{EventUnknown, EventUnknown}, {EventUnknown, EventUnknown}},          // StateCCWStart
	{{EventUnknown, EventUnknown}, {EventUnknown, EventUnknown}},          // StateCCWBothLow
	{{EventUnknown, EventUnknown}, {EventUnknown, EventCounterclockwise}}, // StateCCWFirstHigh

	{{EventUnknown, EventUnknown}, {EventUnknown, EventUnknown}}           // StateInvalid
};

CKY040::TSwitchState CKY040::s_NextSwitchState[SwitchStateUnknown][SwitchEventUnknown] =
{
	// {SwitchEventDown, SwitchEventUp, SwitchEventTick}

	{SwitchStateDown,    SwitchStateStart,  SwitchStateStart},	// SwitchStateStart
	{SwitchStateDown,    SwitchStateClick,  SwitchStateHold},	// SwitchStateDown
	{SwitchStateDown2,   SwitchStateClick,  SwitchStateStart},	// SwitchStateClick
	{SwitchStateDown2,   SwitchStateClick2, SwitchStateInvalid}, 	// SwitchStateDown2
	{SwitchStateDown3,   SwitchStateClick2, SwitchStateStart}, 	// SwitchStateClick2
	{SwitchStateDown3,   SwitchStateClick3, SwitchStateInvalid}, 	// SwitchStateDown3
	{SwitchStateInvalid, SwitchStateClick3, SwitchStateStart}, 	// SwitchStateClick3
	{SwitchStateHold,    SwitchStateStart,  SwitchStateHold},	// SwitchStateHold
	{SwitchStateInvalid, SwitchStateStart,  SwitchStateInvalid}	// SwitchStateInvalid
};

CKY040::TEvent CKY040::s_SwitchOutput[SwitchStateUnknown][SwitchEventUnknown] =
{
	// {SwitchEventDown, SwitchEventUp, SwitchEventTick}

	{EventUnknown, EventUnknown, EventUnknown},		// SwitchStateStart
	{EventUnknown, EventUnknown, EventSwitchHold},		// SwitchStateDown
	{EventUnknown, EventUnknown, EventSwitchClick},		// SwitchStateClick
	{EventUnknown, EventUnknown, EventUnknown},		// SwitchStateDown2
	{EventUnknown, EventUnknown, EventSwitchDoubleClick},	// SwitchStateClick2
	{EventUnknown, EventUnknown, EventUnknown},		// SwitchStateDown3
	{EventUnknown, EventUnknown, EventSwitchTripleClick},	// SwitchStateClick3
	{EventUnknown, EventUnknown, EventSwitchHold},		// SwitchStateHold
	{EventUnknown, EventUnknown, EventUnknown}		// SwitchStateInvalid
};

CKY040::CKY040 (unsigned nCLKPin, unsigned nDTPin, unsigned nSWPin, CGPIOManager *pGPIOManager)
:	m_CLKPin (nCLKPin, GPIOModeInputPullUp, pGPIOManager),
	m_DTPin (nDTPin, GPIOModeInputPullUp, pGPIOManager),
	m_SWPin (nSWPin, GPIOModeInputPullUp, pGPIOManager),
	m_bPollingMode (!pGPIOManager),
	m_bInterruptConnected (FALSE),
	m_pEventHandler (nullptr),
	m_State (StateStart),
	m_hDebounceTimer (0),
	m_hTickTimer (0),
	m_nLastSWLevel (HIGH),
	m_bDebounceActive (FALSE),
	m_SwitchState (SwitchStateStart),
	m_nSwitchLastTicks (0)
{
}

CKY040::~CKY040 (void)
{
	if (m_bInterruptConnected)
	{
		m_pEventHandler = nullptr;

		m_CLKPin.DisableInterrupt2 ();
		m_CLKPin.DisableInterrupt ();
		m_CLKPin.DisconnectInterrupt ();

		m_DTPin.DisableInterrupt2 ();
		m_DTPin.DisableInterrupt ();
		m_DTPin.DisconnectInterrupt ();

		m_SWPin.DisableInterrupt2 ();
		m_SWPin.DisableInterrupt ();
		m_SWPin.DisconnectInterrupt ();
	}

	if (m_hDebounceTimer)
	{
		CTimer::Get ()->CancelKernelTimer (m_hDebounceTimer);
	}

	if (m_hTickTimer)
	{
		CTimer::Get ()->CancelKernelTimer (m_hTickTimer);
	}
}

boolean CKY040::Initialize (void)
{
	if (!m_bPollingMode)
	{
		assert (!m_bInterruptConnected);
		m_bInterruptConnected = TRUE;

		m_CLKPin.ConnectInterrupt (EncoderInterruptHandler, this);
		m_DTPin.ConnectInterrupt (EncoderInterruptHandler, this);
		m_SWPin.ConnectInterrupt (SwitchInterruptHandler, this);

		m_CLKPin.EnableInterrupt (GPIOInterruptOnFallingEdge);
		m_CLKPin.EnableInterrupt2 (GPIOInterruptOnRisingEdge);

		m_DTPin.EnableInterrupt (GPIOInterruptOnFallingEdge);
		m_DTPin.EnableInterrupt2 (GPIOInterruptOnRisingEdge);

		m_SWPin.EnableInterrupt (GPIOInterruptOnFallingEdge);
		m_SWPin.EnableInterrupt2 (GPIOInterruptOnRisingEdge);
	}

	return TRUE;
}

void CKY040::RegisterEventHandler (TEventHandler *pHandler, void *pParam)
{
	assert (!m_pEventHandler);
	m_pEventHandler = pHandler;
	assert (m_pEventHandler);
	m_pEventParam = pParam;
}

unsigned CKY040::GetHoldSeconds (void) const
{
	return m_nHoldCounter / 2;
}

void CKY040::Update (void)
{
	assert (m_bPollingMode);

	EncoderInterruptHandler (this);

	// handle switch
	unsigned nTicks = CTimer::GetClockTicks ();
	unsigned nSW = m_SWPin.Read ();

	if (nSW != m_nLastSWLevel)
	{
		m_nLastSWLevel = nSW;

		m_bDebounceActive = TRUE;
		m_nDebounceLastTicks = CTimer::GetClockTicks ();
	}
	else
	{
		if (   m_bDebounceActive
		    && nTicks - m_nDebounceLastTicks >= SwitchDebounceDelayMillis * (CLOCKHZ / 1000))
		{
			m_bDebounceActive = FALSE;
			m_nSwitchLastTicks = nTicks;

			if (m_pEventHandler)
			{
				(*m_pEventHandler) (nSW ? EventSwitchUp : EventSwitchDown,
						    m_pEventParam);
			}

			HandleSwitchEvent (nSW ? SwitchEventUp : SwitchEventDown);
		}

		if (nTicks - m_nSwitchLastTicks >= SwitchTickDelayMillis * (CLOCKHZ / 1000))
		{
			m_nSwitchLastTicks = nTicks;

			HandleSwitchEvent (SwitchEventTick);
		}
	}
}

// generates the higher level switch events
void CKY040::HandleSwitchEvent (TSwitchEvent SwitchEvent)
{
	assert (SwitchEvent < SwitchEventUnknown);
	TEvent Event = s_SwitchOutput[m_SwitchState][SwitchEvent];
	TSwitchState NextState = s_NextSwitchState[m_SwitchState][SwitchEvent];

	if (NextState == SwitchStateHold)
	{
		if (m_SwitchState != SwitchStateHold)
		{
			m_nHoldCounter = 0;
		}

		m_nHoldCounter++;
	}

	m_SwitchState = NextState;

	if (   Event != EventUnknown
	    && (Event != EventSwitchHold || !(m_nHoldCounter & 1)) // emit hold event each second
	    && m_pEventHandler)
	{
		(*m_pEventHandler) (Event, m_pEventParam);
	}
}

void CKY040::EncoderInterruptHandler (void *pParam)
{
	CKY040 *pThis = static_cast<CKY040 *> (pParam);
	assert (pThis != 0);

	unsigned nCLK = pThis->m_CLKPin.Read ();
	unsigned nDT = pThis->m_DTPin.Read ();
	assert (nCLK <= 1);
	assert (nDT <= 1);

	assert (pThis->m_State < StateUnknown);
	TEvent Event = s_Output[pThis->m_State][nCLK][nDT];
	pThis->m_State = s_NextState[pThis->m_State][nCLK][nDT];

	if (   Event != EventUnknown
	    && pThis->m_pEventHandler)
	{
		(*pThis->m_pEventHandler) (Event, pThis->m_pEventParam);
	}
}

void CKY040::SwitchInterruptHandler (void *pParam)
{
	CKY040 *pThis = static_cast<CKY040 *> (pParam);
	assert (pThis != 0);

	if (pThis->m_hDebounceTimer)
	{
		CTimer::Get ()->CancelKernelTimer (pThis->m_hDebounceTimer);
	}

	pThis->m_hDebounceTimer =
		CTimer::Get ()->StartKernelTimer (MSEC2HZ (SwitchDebounceDelayMillis),
						  SwitchDebounceHandler, pThis, 0);
}

void CKY040::SwitchDebounceHandler (TKernelTimerHandle hTimer, void *pParam, void *pContext)
{
	CKY040 *pThis = static_cast<CKY040 *> (pParam);
	assert (pThis != 0);

	pThis->m_hDebounceTimer = 0;

	if (pThis->m_hTickTimer)
	{
		CTimer::Get ()->CancelKernelTimer (pThis->m_hTickTimer);
	}

	pThis->m_hTickTimer = CTimer::Get ()->StartKernelTimer (MSEC2HZ (SwitchTickDelayMillis),
								SwitchTickHandler, pThis, 0);

	unsigned nSW = pThis->m_SWPin.Read ();

	if (pThis->m_pEventHandler)
	{
		(*pThis->m_pEventHandler) (nSW ? EventSwitchUp : EventSwitchDown,
					   pThis->m_pEventParam);
	}

	pThis->HandleSwitchEvent (nSW ? SwitchEventUp : SwitchEventDown);
}

void CKY040::SwitchTickHandler (TKernelTimerHandle hTimer, void *pParam, void *pContext)
{
	CKY040 *pThis = static_cast<CKY040 *> (pParam);
	assert (pThis != 0);

	pThis->m_hTickTimer = CTimer::Get ()->StartKernelTimer (MSEC2HZ (SwitchTickDelayMillis),
								SwitchTickHandler, pThis, 0);

	pThis->HandleSwitchEvent (SwitchEventTick);
}
