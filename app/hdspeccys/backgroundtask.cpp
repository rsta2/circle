//
// backgroundtask.cpp
//
#include "backgroundtask.h"
#include <circle/sched/scheduler.h>
#include <circle/logger.h>

LOGMODULE ("screenprocessingtask");

#define BACKGROUND_LOOP_PERIOD_MS 500

// Global variable hacks
extern "C" u16 borderValue;
extern "C" u32 videoByteCount;
extern "C" u16 videoValue;
extern "C" u32 frameInterruptCount;
extern "C" u32 skippedFrameCount;
extern "C" u32 screenLoopTicks;
u32 backgroundTime = 0;
u32 prevFrameInterruptCount = 0;


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
		backgroundTime += BACKGROUND_LOOP_PERIOD_MS;
		float fps = (float)((frameInterruptCount - prevFrameInterruptCount) * 1000) / (float)BACKGROUND_LOOP_PERIOD_MS;
		prevFrameInterruptCount = frameInterruptCount;

		// Read the shell (serial port)
		m_pShell->Update();
		// for (unsigned i = 1; i <= 5; i++)
		// {			
		// 	m_pActLED->On ();
		// 	CScheduler::Get ()->MsSleep (200);

		// 	m_pActLED->Off ();
		// 	CScheduler::Get ()->MsSleep (200);
		// }

		
		// u32 len = 0;
		// for (unsigned i = 0; i < (128*1024); i++){
		// 	if (pDEBUG_BUFFER[i] != 0x0000) {
		// 		len++;
		// 	} else {
		// 		break;
		// 	}				
		// }
		// LOGDBG("Buffer len: %04ld", len);
		// LOGDBG("Border value: %04ld", borderValue);				
		// LOGDBG("Video bytes: 0x%04lx, border: 0x%04lx Skipped: 0x%04lx, FPS: %.1f, Loop Time: %.1f ms", 
		// 	videoByteCount, borderValue, skippedFrameCount, fps, (float)screenLoopTicks / 1000);		
		// // if (pDEBUG_BUFFER) {	
		// // 	for (unsigned i = 0; i < 25/*ZX_SMI_DEBUG_BUFFER_LENGTH*/; i++){
		// // 		u16 v = pDEBUG_BUFFER[i];
		// // 		LOGDBG("%04lx", v);
		// // 	}
		// // }
		// if (pVIDEO_BUFFER) {	
		// 	for (unsigned i = 0; i < 25/*ZX_SMI_DEBUG_BUFFER_LENGTH*/; i++){
		// 		u16 v = pVIDEO_BUFFER[i];
		// 		LOGDBG("%04lx", v);
		// 	}
		// }


		CScheduler::Get ()->MsSleep (BACKGROUND_LOOP_PERIOD_MS);
		
	}
}

void CBackgroundTask::Reboot (void* pContext)
{
	CBackgroundTask *pThis = (CBackgroundTask *) pContext;

	// Signal main loop - performs a reboot, by exiting the main loop
	pThis->m_pRebootEvent->Set();
}

