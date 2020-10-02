//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#include "kernel.h"
#include <circle/synchronize.h>

#define SAMPLE_RATE_HZ		25000		// IRQs per second

CKernel *CKernel::s_pThis = 0;

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:
#ifdef USE_BUFFERED_SCREEN
	m_ScreenUnbuffered (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Screen (&m_ScreenUnbuffered),
#else
	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
#endif
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer, TRUE),
	m_Latency (&m_Interrupt)
{
	s_pThis = this;

	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
	s_pThis = 0;
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
#ifdef USE_BUFFERED_SCREEN
		bOK = m_ScreenUnbuffered.Initialize ();
#else
		bOK = m_Screen.Initialize ();
#endif
	}

	if (bOK)
	{
		bOK = m_Logger.Initialize (&m_Screen);
	}

#ifdef USE_BUFFERED_SCREEN
	if (bOK)
	{
		m_Logger.RegisterPanicHandler (PanicHandler);
	}
#endif

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	if (bOK)
	{
		bOK = m_USBHCI.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	m_Latency.Start (SAMPLE_RATE_HZ);

	// start timer to elapse after 5 seconds
	m_Timer.StartKernelTimer (5 * HZ, TimerHandler, 0, this);

	unsigned nTime = m_Timer.GetTime ();
	while (1)
	{
		while (nTime == m_Timer.GetTime ())		// wait a second
		{
			m_USBHCI.UpdatePlugAndPlay ();

#ifdef USE_BUFFERED_SCREEN
			m_Screen.Update ();
#endif
		}

		nTime = m_Timer.GetTime ();

#if 1
		m_Logger.Write (FromKernel, LogNotice, "Maximum IRQ latency was %u us",
				m_Latency.GetMax ());
#else
		m_Latency.Dump ();
#endif
	}

	return ShutdownHalt;
}

void CKernel::TimerHandler (TKernelTimerHandle hTimer, void *pParam, void *pContext)
{
	CKernel *pThis = (CKernel *) pContext;

	// a message from IRQ_LEVEL
	pThis->m_Logger.Write (FromKernel, LogNotice, "Timer elapsed");

	// re-start timer to elapse after 1 second
	pThis->m_Timer.StartKernelTimer (1 * HZ, TimerHandler, 0, pThis);
}

#ifdef USE_BUFFERED_SCREEN

void CKernel::PanicHandler (void)		// called on a system panic condition
{
	EnableIRQs ();				// go to TASK_LEVEL

	s_pThis->m_Screen.Update (2000);	// display all messages before system halt
}

#endif
