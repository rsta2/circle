//
// usertimer.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2018  R. Stange <rsta2@o2online.de>
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
#include <circle/usertimer.h>
#include <circle/bcm2835.h>
#include <circle/bcm2835int.h>
#include <circle/memio.h>
#include <circle/synchronize.h>
#include <assert.h>

CUserTimer::CUserTimer (CInterruptSystem *pInterruptSystem,
			TUserTimerHandler *pHandler, void *pParam,
			boolean bUseFIQ)
:	m_pInterruptSystem (pInterruptSystem),
	m_pHandler (pHandler),
	m_pParam (pParam),
	m_bUseFIQ (bUseFIQ),
	m_bInitialized (FALSE)
{
}

CUserTimer::~CUserTimer (void)
{
	if (m_bInitialized)
	{
		Stop ();
	}

	m_pInterruptSystem = 0;
}

boolean CUserTimer::Initialize (void)
{
	assert (!m_bInitialized);
	m_bInitialized = TRUE;		// asserted in Start()

	Start (3600 * USER_CLOCKHZ);

	assert (m_pInterruptSystem != 0);
	if (!m_bUseFIQ)
	{
		m_pInterruptSystem->ConnectIRQ (ARM_IRQ_TIMER1, InterruptHandler, this);
	}
	else
	{
		m_pInterruptSystem->ConnectFIQ (ARM_FIQ_TIMER1, InterruptHandler, this);
	}

	return TRUE;
}

void CUserTimer::Stop (void)
{
	assert (m_bInitialized);

	if (!m_bUseFIQ)
	{
		m_pInterruptSystem->DisconnectIRQ (ARM_IRQ_TIMER1);
	}
	else
	{
		m_pInterruptSystem->DisconnectFIQ ();
	}

	m_bInitialized = FALSE;
}

void CUserTimer::Start (unsigned nDelayMicros)
{
	assert (m_bInitialized);

	PeripheralEntry ();

	assert (nDelayMicros > 1);
	write32 (ARM_SYSTIMER_C1, read32 (ARM_SYSTIMER_CLO) + nDelayMicros);

	PeripheralExit ();
}

void CUserTimer::InterruptHandler (void *pParam)
{
	CUserTimer *pThis = reinterpret_cast<CUserTimer *> (pParam);
	assert (pThis != 0);

	PeripheralEntry ();
	write32 (ARM_SYSTIMER_CS, 1 << 1);
	PeripheralExit ();

	assert (pThis->m_pHandler != 0);
	(*pThis->m_pHandler) (pThis, pThis->m_pParam);
}
