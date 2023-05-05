//
// backgroundtask.cpp
//
#include "backgroundtask.h"
#include <circle/sched/scheduler.h>

CBackgroundTask::CBackgroundTask (CShell *pShell, CActLED *pActLED, CSynchronizationEvent *pRebootEvent)
:	
	m_pShell (pShell),
	m_pActLED (pActLED),
	m_pRebootEvent (pRebootEvent)
{
}

CBackgroundTask::~CBackgroundTask (void)
{
}

void CBackgroundTask::Run (void)
{
	// Set the reboot callback 
	m_pShell->SetRebootCallback(Reboot, this);

	while (1)
	{
		// Read the shell (serial port)
		m_pShell->Update();
		// for (unsigned i = 1; i <= 5; i++)
		// {			
		// 	m_pActLED->On ();
		// 	CScheduler::Get ()->MsSleep (200);

		// 	m_pActLED->Off ();
		// 	CScheduler::Get ()->MsSleep (200);
		// }

		CScheduler::Get ()->MsSleep (500);
	}
}

void CBackgroundTask::Reboot (void* pContext)
{
	CBackgroundTask *pThis = (CBackgroundTask *) pContext;

	// Signal main loop - performs a reboot, by exiting the main loop
	pThis->m_pRebootEvent->Set();
}

