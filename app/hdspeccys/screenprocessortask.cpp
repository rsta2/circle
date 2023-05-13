//
// screenprocessortask.cpp
//
#include "screenprocessortask.h"
#include <circle/sched/scheduler.h>
#include <circle/logger.h>

LOGMODULE ("screenprocessingtask");


extern "C"	u16 *pDEBUG_BUFFER;
extern "C" u16 borderValue;
extern "C" u32 videoByteCount;

CScreenProcessorTask::CScreenProcessorTask (CZxScreen *pZxScreen, CZxSmi *pZxSmi, CActLED *pActLED)
:	m_FrameEvent(0),
	m_pZxScreen (pZxScreen),
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

	m_pZxSmi->Start(&m_FrameEvent);
	m_pZxScreen->Start();

	while (1)
	{	
		// m_FrameEvent.Clear();
		// m_FrameEvent.Wait();

		// CScheduler::Get ()->Yield ();

		// // CScheduler::Get ()->MsSleep (5000);

		// // // m_ZxSmi.Start();
		// ZX_DMA_T value = m_pZxSmi->GetValue();
		// // LOGDBG("DATA: %04lx", value);

		// // // ZX_DMA_T value2 = m_pZxSmi->m_counter2;
		// // // LOGDBG("Reg: %08lx", value2);

		// // // for (unsigned i = 0; i < 25/*ZX_SMI_DEBUG_BUFFER_LENGTH*/; i++){
		// // // 	ZX_DMA_T v =m_pZxSmi->m_pDebugBuffer[i];
		// // // 	LOGDBG("%04lx", v);
		// // // }

		
		// ZX_DMA_T * pBuffer = m_pZxSmi->GetScreenDataBuffer();
		// pDEBUG_BUFFER = pBuffer;
		// u32 bufferLen = m_pZxSmi->GetScreenDataBufferLength();


		// // Reset state before processing data (this interrupt occurs at start of screen refresh)
		// u32 lastValue = 0;
		// u32 inIOWR = 0;
		// u32 inCAS = 0;
		// videoByteCount = 0;
		// u32 screenValue0 = 0;
		// u32 screenValue1 = 0;
		// u32 casReadIdx = 0;

		// // Loop all the data in the buffer
		// for (unsigned i = 0; i < bufferLen; i++) {   
		// 	// Read the value from the buffer
		// 	ZX_DMA_T value = pBuffer[i];

		// 	// if (value != 0x0000) {
		// 	// pThis->m_value = value;

		// 	boolean IORQ = (value & (1 << 0)) == 0;
		// 	boolean WR = (value & (1 << 1)) == 0;
		// 	boolean A0 = (value & (1 << 2)) != 0;
		// 	boolean CAS = (value & (1 << 3)) == 0;
		// 	boolean IOREQ = IOREQ & A0;                
		// 	u32 data = (value >> 8) & 0x7;

		// 	// Test
		// 	// if (CAS) {
		// 	//   pThis->m_value = 0xDDDD;
		// 	// }

		// 	// Handle ULA video read (/CAS in close succession (~100ns in-between each cas)
		// 	if (i > 10000) {
		// 		if (CAS) {
		// 			// Entered or in CAS
		// 			inCAS++;
		// 		} else {
		// 			// if (inCAS == 3) {
		// 			//   if (casReadIdx == 0) {
		// 			//     screenValue0 = lastValue;
		// 			//   } else {
		// 			//     screenValue1 = lastValue;
		// 			//     videoByteCount +=2;
		// 			//   }
		// 			// } else if (inCAS > 3) {
		// 			//   casReadIdx = 0;
		// 			// }
		// 			if (inCAS == 3) {
		// 			// // pThis->m_value = lastValue;   
		// 			videoByteCount++;  
		// 			// pThis->m_value = inCAS;   
		// 			// pThis->m_value =  0xDDDD;                 
		// 			} 
		// 			inCAS = 0;
		// 		}
		// 	}

		// 	// Handle IO write (border colour)
		// 	if (IORQ & WR) {          
		// 		// Entered or in IO write
		// 		inIOWR++;          
		// 	} else {
		// 		if (inIOWR > 5) {
		// 			// Exited IO write, use the last value
		// 			borderValue = lastValue;
		// 			// pThis->m_value =  0xDDDD;
		// 		}
		// 		inIOWR = 0;
		// 	}


		// 	lastValue = data;

		// 	// // Check for /IOREQ = 0 (/IOREQ = /IORQ | /A0)
		// 	// if (!pThis->m_isIOWrite && (value & ((1 << 15) | (1 << 13))) == 0) {
		// 	//   // pThis->m_value = value; // & 0x07;  // Only interested in first 3 bits
		// 	//   pThis->m_isIOWrite = true;        
		// 	//   pThis->m_counter2 = 0;
		// 	//   }      
		// 	//   else if (pThis->m_isIOWrite && (value & ((1 << 15) | (1 << 14) | (1 << 13))) == 0) {
		// 	//   // pThis->m_value = value & 0x07;  // Only interested in first 3 bits for the border colour
		// 	//   pThis->m_counter2++;

		// 	//   if (pThis->m_counter2 > 5) {
		// 	//   pThis->m_value = value;
		// 	//   pThis->m_isIOWrite = false;        
		// 	//   }
		// 	// }
			
		// 	// if (value > 0) {
		// 	//   pThis->m_counter++;
		// 	// }
		// }

		

		ZX_DMA_T value = borderValue;

		// m_Screen.SetPixel(x++,y++, RED_COLOR);
		value = value & 0x7;
		TScreenColor bc =BRIGHT_MAGENTA_COLOR;// clear ? MAGENTA_COLOR : GREEN_COLOR; //BLACK_COLOR;
		if (value == 0) bc = BLACK_COLOR;
		if (value == 1) bc = BLUE_COLOR;
		if (value == 2) bc = RED_COLOR;
		if (value == 3) bc = MAGENTA_COLOR;
		if (value == 4) bc = GREEN_COLOR;
		if (value == 5) bc = CYAN_COLOR;
		if (value == 6) bc = YELLOW_COLOR;
		if (value == 7) bc = WHITE_COLOR;

		// m_pZxScreen->SetBorder(bc);
		// m_pZxScreen->SetScreen(TRUE);
		m_pZxScreen->SetScreenFromBuffer((u16 *)m_pZxSmi->GetScreenDataBuffer(), 0x3000 / 2);

		m_pZxScreen->UpdateScreen();

		// clear = !clear;

		// // LOGDBG("nSourceAddress: 0x%08lx", m_ZxSmi.m_src);
		// // LOGDBG("status: 0x%08lx", m_ZxSmi.m_src);
		// // LOGDBG("nDestinationAddress: 0x%08lx", m_ZxSmi.m_dst);
		// // LOGDBG(".");
		
		// // CScheduler::Get ()->MsSleep (500);
		CScheduler::Get ()->Yield ();
		// // m_Timer.MsDelay(500);
		// // m_Timer.MsDelay(1);
		// CScheduler::Get ()->MsSleep (500);
	}
}
