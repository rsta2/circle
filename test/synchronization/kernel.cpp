//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2018  R. Stange <rsta2@o2online.de>
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
#include <circle/string.h>
#include <assert.h>

static const char FromKernel[] = "kernel";

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

class CWorkerTask : public CTask
{
public:
	CWorkerTask(int id, int timeout, CSynchronizationEvent* pEvent)
	{
		m_id = id;
		m_timeout = timeout;
		m_pEvent = pEvent;
	}

	virtual void Run() override
	{
		CLogger::Get()->Write (FromKernel, LogNotice, "Worker %i waiting for event with timeout %ims...", m_id, m_timeout);
		while (!m_pEvent->GetState())
		{
			if (m_pEvent->WaitWithTimeout(m_timeout * 1000))
				CLogger::Get()->Write (FromKernel, LogNotice, "Worker %i timed out", m_id);
		}
		CLogger::Get()->Write (FromKernel, LogNotice, "Worker %i signalled!", m_id);
	}

	int m_id;
	int m_timeout;
	CSynchronizationEvent* m_pEvent;
};

class CCounterTask : public CTask
{
public:
	CCounterTask(int id, int timeout, int* piCounter, CMutex* pMutex)
	{
		m_id = id;
		m_timeout = timeout;
		m_piCounter = piCounter;
		m_pMutex = pMutex;
	}

	virtual void Run() override
	{
		for (int i=0; i<10; i++)
		{
			// Simulates an atomic operation with intervening sleep

			// Get mutex
			CLogger::Get()->Write (FromKernel, LogNotice, "Counter task %i iter %i", m_id, i);
			m_pMutex->Acquire();
			CLogger::Get()->Write (FromKernel, LogNotice, "Counter task %i mutex acquired", m_id);

			// Part 1 of atomic op
			int value = *m_piCounter;

			// No one ele should get in while we sleep
			CScheduler::Get()->MsSleep(m_timeout);

			// Part 2 of atomic op
			*m_piCounter = value + 1;

			// Give someone else a go...
			CLogger::Get()->Write (FromKernel, LogNotice, "Counter task %i releasing mutex", m_id);
			m_pMutex->Release();
		}
	
		CLogger::Get()->Write (FromKernel, LogNotice, "Counter %i finished.", m_id);
	}
	

	int m_id;
	int m_timeout;
	int* m_piCounter;
	CMutex* m_pMutex;
};

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	// Example 1 - Wait with Timeout
	m_Logger.Write (FromKernel, LogNotice, "\n\n### Example 1 - Wait with Timeout ###");
	m_Timer.StartKernelTimer (5 * HZ, TimerHandler, this);	// 5 seconds
	m_Logger.Write (FromKernel, LogNotice, "Event will trigger in 5 seconds");
	m_Logger.Write (FromKernel, LogNotice, "Waiting for event with 1 second timeout...");
	while (!m_Event.GetState())
	{
		if (m_Event.WaitWithTimeout(1000000))		// 1 second
		{
			m_Logger.Write (FromKernel, LogNotice, "  time out!");
		}
	}
	m_Logger.Write (FromKernel, LogNotice, "Event signalled");
	m_Event.Clear();

	// Example 2 - Multi-task Wait (some with timeouts)
	m_Logger.Write (FromKernel, LogNotice, "\n\n### Example 2 - Multi-task Wait ###");
	new CWorkerTask(1, 0, &m_Event);
	new CWorkerTask(2, 300, &m_Event);
	new CWorkerTask(3, 600, &m_Event);
	new CWorkerTask(4, 900, &m_Event);
	new CWorkerTask(0, 0, &m_Event);
	m_Logger.Write (FromKernel, LogNotice, "Event will trigger in 5 seconds...");
	CScheduler::Get()->MsSleep(5000);
	m_Logger.Write (FromKernel, LogNotice, "Triggering event...");
	m_Event.Set();
	m_Logger.Write (FromKernel, LogNotice, "Event triggered");
	CScheduler::Get()->Yield();

	// Example 3 - Mutexes
	m_Logger.Write (FromKernel, LogNotice, "\n\n### Example 3 - Mutexes ###");
	int counter = 0;
	new CCounterTask(1, 10, &counter, &m_Mutex);
	new CCounterTask(2, 20, &counter, &m_Mutex);
	new CCounterTask(3, 30, &counter, &m_Mutex);
	new CCounterTask(4, 40, &counter, &m_Mutex);
	new CCounterTask(5, 50, &counter, &m_Mutex);
	CScheduler::Get()->MsSleep(3000);		// Approx how long above tasks will take
	m_Logger.Write (FromKernel, LogNotice, "Final counter: %i (should be 50)", counter);
	assert(counter == 50);


	m_Logger.Write (FromKernel, LogNotice, "Finished!");
	CScheduler::Get()->MsSleep(1000);
	return ShutdownHalt;
}

void CKernel::TimerHandler (TKernelTimerHandle hTimer, void *pParam, void *pContext)
{
	CKernel *pThis = (CKernel *) pParam;
	assert (pThis != 0);
	pThis->m_Event.Set ();
}
