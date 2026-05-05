//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2026  R. Stange <rsta2@o2online.de>
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
#include "writertask.h"
#include "readertask.h"
#include "config.h"
#include <circle/sched/pipe.h>
#include <assert.h>

LOGMODULE ("kernel");

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer)
{
	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Serial.Initialize (115200);
	}

	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Screen;
		}

		bOK = m_Logger.Initialize (pTarget);
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	LOGNOTE ("Compile time: " __DATE__ " " __TIME__);

	// Create pipe
	CPipe *pPipe = new CPipe (FALSE, PIPE_SIZE, ATOMIC_WRITE);
	assert (pPipe);

	// Start tasks
	CTask *pWriter[NUM_WRITERS];
	for (unsigned i = 0; i < NUM_WRITERS; i++)
	{
		pWriter[i] = new CWriterTask (pPipe->GetWriter (), m_Logger.GetTarget ());
		assert (pWriter[i]);
	}

	CTask *pReader[NUM_READERS];
	for (unsigned i = 0; i < NUM_READERS; i++)
	{
		pReader[i] = new CReaderTask (pPipe->GetReader (), m_Logger.GetTarget ());
		assert (pReader[i]);
	}

	// Wait for task termination
	for (unsigned i = 0; i < NUM_WRITERS; i++)
	{
		pWriter[i]->WaitForTermination ();
	}

	for (unsigned i = 0; i < NUM_READERS; i++)
	{
		pReader[i]->WaitForTermination ();
	}

	LOGNOTE ("Test completed");

	return ShutdownHalt;
}
