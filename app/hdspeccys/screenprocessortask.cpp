//
// screenprocessortask.cpp
//
#include "screenprocessortask.h"
#include <circle/sched/scheduler.h>
#include <circle/logger.h>

LOGMODULE ("screenprocessingtask");


extern "C" u32 screenLoopCount;
extern "C" u32 screenLoopTicks;
extern "C" u32 ulaBufferTicks;
extern "C" u32 screenUpdateTicks;

// unsigned char reverseBits(unsigned char b) {
//    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
//    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
//    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
//    return b;
// }



CScreenProcessorTask::CScreenProcessorTask (CZxScreen *pZxScreen, CZxSmi *pZxSmi, CActLED *pActLED)
:	m_FrameEvent(0),
	m_pZxScreen (pZxScreen),
	m_pZxSmi (pZxSmi),
	m_pActLED (pActLED),
	m_pScreenPixelDataBuffer(0),
	m_pScreenAttrDataBuffer(0),
	m_nScreenDataBufferLength(0)
{
	// TODO: This is way too big
	m_nScreenDataBufferLength = ZX_DMA_BUFFER_LENGTH * sizeof(ZX_DMA_T);  

	// Allocate screen data buffers (Do in run?)
  	m_pScreenPixelDataBuffer = new ZX_DMA_T[m_nScreenDataBufferLength];
	m_pScreenAttrDataBuffer = new ZX_DMA_T[m_nScreenDataBufferLength];
}

CScreenProcessorTask::~CScreenProcessorTask (void)
{
  // Deallocate screen data buffer
  delete m_pScreenPixelDataBuffer;
  m_pScreenPixelDataBuffer = nullptr;
  delete m_pScreenAttrDataBuffer;
  m_pScreenAttrDataBuffer = nullptr;
}

void CScreenProcessorTask::Run (void)
{
	// boolean clear = TRUE;

	m_pZxSmi->Start(&m_FrameEvent);
	m_pZxScreen->Start();

	while (1)
	{	
		m_FrameEvent.Clear();
		m_FrameEvent.Wait();

		unsigned startTicks = CTimer::Get()->GetClockTicks();

		// Get pointer to screen buffer (will be null if no new frame available)
		ZX_DMA_T *pULABuffer = m_pZxSmi->LockDataBuffer();

		if (pULABuffer) {		
			// ZX_DMA_T value = borderValue;

			// // m_Screen.SetPixel(x++,y++, RED_COLOR);
			// value = value & 0x7;
			// TScreenColor bc =BRIGHT_MAGENTA_COLOR;// clear ? MAGENTA_COLOR : GREEN_COLOR; //BLACK_COLOR;
			// if (value == 0) bc = BLACK_COLOR;
			// if (value == 1) bc = BLUE_COLOR;
			// if (value == 2) bc = RED_COLOR;
			// if (value == 3) bc = MAGENTA_COLOR;
			// if (value == 4) bc = GREEN_COLOR;
			// if (value == 5) bc = CYAN_COLOR;
			// if (value == 6) bc = YELLOW_COLOR;
			// if (value == 7) bc = WHITE_COLOR;

			// m_pZxScreen->SetBorder(bc);
			// // m_pZxScreen->SetScreen(TRUE);


			// Set screen data from ULA buffer
			m_pZxScreen->SetScreenFromULABuffer(pULABuffer, ZX_DMA_BUFFER_LENGTH);
		}

		// Release the SMI DMA buffer pointer
		m_pZxSmi->ReleaseDataBuffer();

		// Timing
		ulaBufferTicks = (CTimer::Get()->GetClockTicks() - startTicks);

		// Update the screen
		m_pZxScreen->UpdateScreen();

		// Time the loop
		screenLoopCount++;
		
		// Timing		
		screenUpdateTicks = (CTimer::Get()->GetClockTicks() - startTicks) - ulaBufferTicks;
		screenLoopTicks = ulaBufferTicks + screenUpdateTicks;
	

		// clear = !clear;

		// // LOGDBG("nSourceAddress: 0x%08lx", m_ZxSmi.m_src);
		// // LOGDBG("status: 0x%08lx", m_ZxSmi.m_src);
		// // LOGDBG("nDestinationAddress: 0x%08lx", m_ZxSmi.m_dst);
		// // LOGDBG(".");
		
		// // CScheduler::Get ()->MsSleep (500);
		// CScheduler::Get ()->Yield ();
		// // m_Timer.MsDelay(500);
		// // m_Timer.MsDelay(1);
		// CScheduler::Get ()->MsSleep (20);
	}
}
 