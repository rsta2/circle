//
// screenprocessortask.cpp
//
#include "tapetask.h"
#include <circle/sched/scheduler.h>
#include <circle/logger.h>

LOGMODULE ("tapetask");




CTapeTask::CTapeTask (CZxTape *pZxTape)
:	m_pZxTape (pZxTape)	
{
	//
}

CTapeTask::~CTapeTask (void)
{
  //
}

void CTapeTask::Run (void)
{
	// Init

	// HACK
	m_pZxTape->PlayPause();

	while (1)
	{	
		m_pZxTape->Update();


		// Not sure how often we will need to call the Tape loop to keep up with playing
		// The buffer that is read into is 64 words (128 bytes) long
		CScheduler::Get ()->MsSleep (50);
		// CScheduler::Get ()->Yield ();
		// // m_Timer.MsDelay(500);
		// // m_Timer.MsDelay(1);
		// CScheduler::Get ()->MsSleep (20);
	}
}
 