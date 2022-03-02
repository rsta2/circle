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

static const unsigned SwitchDebounceDelayHZ = MSEC2HZ (50);

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

CKY040::CKY040 (CGPIOManager *pGPIOManager, unsigned nCLKPin, unsigned nDTPin, unsigned nSWPin)
:	m_CLKPin (nCLKPin, GPIOModeInputPullUp, pGPIOManager),
	m_DTPin (nDTPin, GPIOModeInputPullUp, pGPIOManager),
	m_SWPin (nSWPin, GPIOModeInputPullUp, pGPIOManager),
	m_pEventHandler (nullptr),
	m_State (StateStart),
	m_hDebounceTimer (0)
{
}

CKY040::~CKY040 (void)
{
	if (m_pEventHandler)
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
}

void CKY040::RegisterEventHandler (TEventHandler *pHandler, void *pParam)
{
	assert (!m_pEventHandler);
	m_pEventHandler = pHandler;
	assert (m_pEventHandler);
	m_pEventParam = pParam;

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

void CKY040::EncoderInterruptHandler (void *pParam)
{
	CKY040 *pThis = static_cast<CKY040 *> (pParam);
	assert (pThis != 0);

	unsigned nCLK = pThis->m_CLKPin.Read ();
	unsigned nDT = pThis->m_DTPin.Read ();
	assert (nCLK <= 1);
	assert (nDT <= 1);

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

	pThis->m_hDebounceTimer = CTimer::Get ()->StartKernelTimer (SwitchDebounceDelayHZ,
								    SwitchTimerHandler, pThis, 0);
}

void CKY040::SwitchTimerHandler (TKernelTimerHandle hTimer, void *pParam, void *pContext)
{
	CKY040 *pThis = static_cast<CKY040 *> (pParam);
	assert (pThis != 0);

	pThis->m_hDebounceTimer = 0;

	if (pThis->m_pEventHandler)
	{
		(*pThis->m_pEventHandler) (  pThis->m_SWPin.Read ()
					   ? EventSwitchUp
					   : EventSwitchDown,
					   pThis->m_pEventParam);
	}
}
