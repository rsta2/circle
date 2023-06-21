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

#if 0
	// HACK
	m_pZxTape->PlayPause();

	while (1)
	{	
		m_pZxTape->Update();

		if (m_pZxTape->IsPlaying()) {
			// If playing, loop as fast as possible
			CScheduler::Get ()->Yield ();
		} else {
			// If not playing, loop more slowly to save CPU
			CScheduler::Get ()->MsSleep (250);
		}		
	}
#endif	
}
 