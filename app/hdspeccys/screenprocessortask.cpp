//
// screenprocessortask.cpp
//
#include "screenprocessortask.h"
#include <circle/sched/scheduler.h>
#include <circle/logger.h>

LOGMODULE ("screenprocessingtask");




CScreenProcessorTask::CScreenProcessorTask (CZxScreen *pZxScreen, CZxSmi *pZxSmi, CActLED *pActLED)
:	m_pZxScreen (pZxScreen),
	m_pZxSmi (pZxSmi),
	m_pActLED (pActLED)
{
}

CScreenProcessorTask::~CScreenProcessorTask (void)
{
}

void CScreenProcessorTask::Run (void)
{
	boolean clear = TRUE;

	m_pZxSmi->Start();
	m_pZxScreen->Start();

	while (1)
	{	
		// CScheduler::Get ()->MsSleep (5000);

		// m_ZxSmi.Start();
		ZX_DMA_T value = m_pZxSmi->GetValue();
		LOGDBG("DATA: %04lx", value);

		// ZX_DMA_T value2 = m_pZxSmi->m_counter2;
		// LOGDBG("Reg: %08lx", value2);

		for (unsigned i = 0; i < 25/*ZX_SMI_DEBUG_BUFFER_LENGTH*/; i++){
			ZX_DMA_T v =m_pZxSmi->m_pDebugBuffer[i];
			LOGDBG("%04lx", v);
		}

		

		// m_Screen.SetPixel(x++,y++, RED_COLOR);
		value = value & 0x7;
		TScreenColor bc =MAGENTA_COLOR;// clear ? MAGENTA_COLOR : GREEN_COLOR; //BLACK_COLOR;
		if (value == 1) bc = BLUE_COLOR;
		if (value == 2) bc = RED_COLOR;
		if (value == 3) bc = MAGENTA_COLOR;
		if (value == 4) bc = GREEN_COLOR;
		if (value == 5) bc = CYAN_COLOR;
		if (value == 6) bc = YELLOW_COLOR;
		if (value == 7) bc = WHITE_COLOR;

		m_pZxScreen->SetBorder(bc);
		m_pZxScreen->SetScreen(TRUE);

		m_pZxScreen->UpdateScreen();

		clear = !clear;

		// LOGDBG("nSourceAddress: 0x%08lx", m_ZxSmi.m_src);
		// LOGDBG("status: 0x%08lx", m_ZxSmi.m_src);
		// LOGDBG("nDestinationAddress: 0x%08lx", m_ZxSmi.m_dst);
		// LOGDBG(".");
		
		// CScheduler::Get ()->MsSleep (500);
		// CScheduler::Get ()->Yield ();
		// m_Timer.MsDelay(500);
		// m_Timer.MsDelay(1);
		CScheduler::Get ()->MsSleep (500);
	}
}
