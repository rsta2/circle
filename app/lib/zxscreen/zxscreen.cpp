//
// zxscreen.cpp
//
#include <zxscreen/zxscreen.h>


LOGMODULE ("ZxScreen");


//
// Class
//

CZxScreen::CZxScreen (unsigned nWidth, unsigned nHeight,
		                  boolean bDoubleBuffer, unsigned nDisplay, CInterruptSystem *pInterruptSystem)
: m_pInterruptSystem (pInterruptSystem),
  m_pFrameBuffer (0),
  m_nInitWidth (nWidth),
	m_nInitHeight (nHeight),  
  m_bDoubleBuffer(FALSE),
	m_nDisplay (nDisplay),	
	m_pBuffer (0),	
  m_pOffscreenBuffer (0),
  m_nSize (0),
	m_nPitch (0),
	m_nWidth (0),
	m_nHeight (0),
  m_nBorderWidth (0),
	m_nBorderHeight (0),
  m_nScreenWidth (0),
	m_nScreenHeight (0),
	m_bUpdated (FALSE),
  m_bRunning(FALSE),  
  m_pActLED(0)
  // m_bDMAResult(0),
  // m_counter(0),
  // m_value(0),
  // m_isIOWrite(0)
{
  
}

CZxScreen::~CZxScreen (void)
{
  m_pBuffer = 0;
	
	delete m_pFrameBuffer;
	m_pFrameBuffer = 0;
}

boolean CZxScreen::Initialize ()
{
  // unsigned i;
  // unsigned dmaBufferLenWords = ZX_DMA_BUFFER_LENGTH;
  // unsigned dmaBufferLenBytes = ZX_DMA_BUFFER_LENGTH * sizeof(ZX_DMA_T);
  
  LOGNOTE("Initializing ZX SCREEN");

  m_nScreenWidth = ZX_SCREEN_PIXEL_WIDTH;
  m_nScreenHeight = ZX_SCREEN_PIXEL_HEIGHT;
  m_nBorderWidth = (m_nInitWidth - m_nScreenWidth) / 2;
  m_nBorderHeight = (m_nInitHeight - m_nScreenHeight) / 2;

  // Create the framebuffer
  m_pFrameBuffer = new CBcmFrameBuffer (m_nInitWidth, m_nInitHeight, DEPTH,
						      0, 0, m_nDisplay, m_bDoubleBuffer);

  // Initialize the framebuffer
  if (!m_pFrameBuffer->Initialize ())
  {
    return FALSE;
  }

  // Check depth is correct
  if (m_pFrameBuffer->GetDepth () != DEPTH)
  {
    return FALSE;
  }

  // Get pointer to actual buffer
  m_pBuffer = (TScreenColor *) (uintptr) m_pFrameBuffer->GetBuffer ();

  // Get screen / buffer size
  m_nSize   = m_pFrameBuffer->GetSize ();
  m_nPitch  = m_pFrameBuffer->GetPitch ();
  m_nWidth  = m_pFrameBuffer->GetWidth ();
  m_nHeight = m_pFrameBuffer->GetHeight ();

  // Ensure that each row is word-aligned so that we can safely use memcpyblk()
  if (m_nPitch % sizeof (u32) != 0)
  {
    return FALSE;
  }
  m_nPitch /= sizeof (TScreenColor);


  // Clear the screen
  Clear(WHITE_COLOR);


  // PeripheralEntry ();
  
  // PeripheralExit ();

	return TRUE;
}

void CZxScreen::Start()
{
  LOGDBG("Starting ZX SCREEN");

  // Set running flag
  m_bRunning = TRUE;

  // // Set initial DMA buffer
  // m_pDMABuffer = m_pDMABuffers[m_DMABufferIdx];

  // // Set DMA completion routine
  // m_DMA.SetCompletionRoutine(DMACompleteInterrupt, this);
  // // m_DMA_2.SetCompletionRoutine(DMACompleteInterrupt, this);
  
  // DMAStart();  

  // if (m_pActLED != nullptr) {
  //   m_pActLED->On();
  // }

  // m_DMA.Wait();
}

void CZxScreen::Stop()
{
  LOGDBG("Stopping ZX SCREEN");

  m_bRunning = FALSE;

  // TODO - stop the hardware / cancel the DMA if possible
}

void CZxScreen::SetActLED(CActLED *pActLED) {
  m_pActLED = pActLED;
}

unsigned CZxScreen::GetWidth (void) const
{
	return m_nWidth;
}

unsigned CZxScreen::GetHeight (void) const
{
	return m_nHeight;
}

// CBcmFrameBuffer *CZxScreen::GetFrameBuffer (void)
// {
// 	return m_pFrameBuffer;
// }

void CZxScreen::Clear (TScreenColor backgroundColor)
{	
	TScreenColor *pBuffer = m_pBuffer;
	unsigned nSize = m_nSize / sizeof (TScreenColor);
	
	while (nSize--)
	{
		*pBuffer++ = backgroundColor;
	}
}

void CZxScreen::SetBorder (TScreenColor borderColor)
{	
	TScreenColor *pBuffer = m_pBuffer;
	unsigned nSize = m_nSize / sizeof (TScreenColor);
  unsigned x = 0;
  unsigned y = 0;
	
	while (nSize--)
	{
    // TScreenColor color = WHITE_COLOR;

    if (x < m_nBorderWidth || x >= m_nWidth - m_nBorderWidth || 
        y < m_nBorderHeight || y >= m_nHeight - m_nBorderHeight) {
      // Set pixel to colour
      *pBuffer = borderColor;
    }

    // Increment pixel pointer
    pBuffer++;

    // Track pixel position in buffer
    x++;
    if (x >= m_nWidth) {
      x = 0;
      y++;
      if (y >= m_nHeight) {
        y = 0;
      }
    }
	}
}

// TODO - double buffering, and then DMA
void CZxScreen::SetScreen (boolean bToggle)
{
	TScreenColor *pBuffer = m_pBuffer;
	unsigned nSize = m_nSize / sizeof (TScreenColor);
  unsigned x = 0;
  unsigned y = 0;
  unsigned px = 0;
  unsigned py = 0;
  boolean bInScreen = FALSE;
	
	while (nSize--)
	{
    px = x - m_nBorderWidth;
    py = y - m_nBorderHeight;
    bInScreen = (px >= 0 && px < (m_nScreenWidth)) && 
                (py >= 0 && py < (m_nScreenHeight));

    // In the screen
    if (bInScreen) { 
      *pBuffer = bToggle ? rand_32() & 0xFFFF : WHITE_COLOR;
    }


    // Increment pixel pointer
    pBuffer++;

    // Track pixel position in buffer
    x++;
    if (x >= m_nWidth) {
      x = 0;
      y++;
      if (y >= m_nHeight) {
        y = 0;
      }
    }
	}
}

//
// Private
//

void CZxScreen::DMAStart()
{
  // CDMAChannel dma = m_DMABufferIdx == 0 ? m_DMA : m_DMA_2;

  // m_SMIMaster.StartDMA(m_DMA, m_pDMABuffer);  

  // if (m_pActLED != nullptr) {
  //   m_pActLED->On();
  // }

}

void CZxScreen::DMACompleteInterrupt(unsigned nChannel, boolean bStatus, void *pParam)
{
  // unsigned i = 0;

  // CZxScreen *pThis = (CZxScreen *) pParam;
	// assert (pThis != 0);

  // // if (pThis->m_pActLED != nullptr) {
  // //   pThis->m_pActLED->Off();
  // // }

  // // Return early if stopped
  // if (!pThis->m_bRunning) return;

  // // CDMAChannel dma = pThis->m_DMABufferIdx == 0 ? pThis->m_DMA : pThis->m_DMA_2;

  // // Save pointer to buffer just filled by DMA
  // ZX_DMA_T *pBuffer = pThis->m_pDMABuffer;

  // // Move to the next DMA buffer
  // pThis->m_DMABufferIdx = (pThis->m_DMABufferIdx + 1) % ZX_DMA_BUFFER_COUNT;
  // pThis->m_pDMABuffer = pThis->m_pDMABuffers[pThis->m_DMABufferIdx];

  // // (Re-)Start the DMA
  // pThis->DMAStart();


  // Process the buffer (TODO - this should happen in a bottom half)
  // boolean have1 = FALSE;
  // for (unsigned i = 0; i < ZX_DMA_BUFFER_LENGTH; i++) {    
  //   if (pBuffer[i] > 0) {
  //     have1 = TRUE;
  //   }
  // }

  // if (have1) {
  //   pThis->m_bDMAResult = !pThis->m_bDMAResult;
  // }

  // if (pThis->m_bDMAResult) {
  //   pThis->m_pActLED->On();
  // } else {
  //   pThis->m_pActLED->Off();
  // }

  // for (unsigned i = 0; i < ZX_DMA_BUFFER_LENGTH; i++) {   
  //   ZX_DMA_T value = pBuffer[i];
  //   if (value > 0) {
  //     pThis->m_counter++;

  //     // pThis->m_value = value;


  //     // TODO: BORDER detection is sort of working, but I have reduced the SMI clock duration
  //     // Instead, there should be an IOWrite counter.

  //     // INT length is 282ns (One scan line is 64us)

  //     // CAS ULA detection works by measuring time between 2 CAS pulses. Should be between 
  //     // 200ns - 400ns, but NOT longer, otherwise it's a CPU read

  //     // Check for /IOREQ = 0 (/IOREQ = /IORQ | /A0)
  //     if (!pThis->m_isIOWrite && (value & ((1 << 15) | (1 << 13))) == 0) {
  //       // pThis->m_value = value; // & 0x07;  // Only interested in first 3 bits
  //       pThis->m_isIOWrite = true;        
  //     }      
  //     else if (pThis->m_isIOWrite && (value & ((1 << 15) | (1 << 14) | (1 << 13))) == 0) {
  //       // pThis->m_value = value & 0x07;  // Only interested in first 3 bits for the border colour
  //       pThis->m_value = value;
  //       pThis->m_isIOWrite = false;        
  //     }    
  //   }
  // }
  
  // if (pThis->m_counter > 10000000) {
  //   pThis->m_counter = 0;
  //   pThis->m_bDMAResult = !pThis->m_bDMAResult;
   
  //   if (pThis->m_bDMAResult) {
  //     pThis->m_pActLED->On();
  //   } else {
  //     pThis->m_pActLED->Off();
  //   }
  // }

}





