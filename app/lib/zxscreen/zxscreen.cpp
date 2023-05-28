//
// zxscreen.cpp
//
// TODO
// - I am not sure if it is faster to use the offscreen buffer and DMA, or just write 
//   directly to the offscreen part of the frame buffer? (and then wouldn't need DMA)
//   - need to test above
//
#include <circle/new.h>
#include <zxscreen/zxscreen.h>


LOGMODULE ("ZxScreen");

#define SCREEN_BUFFER_COUNT 2
// #define SPINLOCK_LEVEL      IRQ_LEVEL /* TASK_LEVEL*/
#define SPINLOCK_LEVEL      TASK_LEVEL

//
// Class
//

CZxScreen::CZxScreen (unsigned nWidth, unsigned nHeight, unsigned nDisplay, CInterruptSystem *pInterruptSystem)
: m_pInterruptSystem (pInterruptSystem),
  m_pFrameBuffer (0),
  m_nInitWidth (nWidth),
	m_nInitHeight (nHeight),  
	m_nDisplay (nDisplay),	
  m_nPitch (0),
	m_nWidth (0),
	m_nHeight (0),
  m_nBorderWidth (0),
	m_nBorderHeight (0),
  m_nScreenWidth (0),
	m_nScreenHeight (0),
  m_pScreenBuffer (0),	
  m_pOffscreenBuffer (0),
  // m_pOffscreenBuffer2 (0),
  m_nScreenBufferSize (0),
  m_nOffscreenBufferSize (0),
  m_nScreenBufferNo (0),
	m_bDirty (FALSE),
#ifdef ZX_SCREEN_DMA  
  m_DMA(DMA_CHANNEL_NORMAL),
  // m_pDMAControlBlock(0),
  // m_pDMAControlBlock2(0),
  // m_pDMAControlBlock3(0),
  // m_pDMAControlBlock4(0),
#endif  
  m_SpinLock(SPINLOCK_LEVEL),
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
  // m_DMA.FreeDMAControlBlock(m_pDMAControlBlock);
  // m_DMA.FreeDMAControlBlock(m_pDMAControlBlock2);
  // m_DMA.FreeDMAControlBlock(m_pDMAControlBlock3);
  // m_DMA.FreeDMAControlBlock(m_pDMAControlBlock4);

  m_pScreenBuffer = 0;

  delete m_pOffscreenBuffer;
  m_pOffscreenBuffer = 0;

  // delete m_pOffscreenBuffer2;
  // m_pOffscreenBuffer2 = 0;
	
	delete m_pFrameBuffer;
	m_pFrameBuffer = 0;
}

boolean CZxScreen::Initialize ()
{
  // unsigned i;
  // unsigned dmaBufferLenWords = ZX_DMA_BUFFER_LENGTH;
  // unsigned dmaBufferLenBytes = ZX_DMA_BUFFER_LENGTH * sizeof(ZX_DMA_T);
  
  LOGNOTE("Initializing ZX SCREEN");

  // m_pDMAControlBlock = m_DMA.CreateDMAControlBlock();
  // m_pDMAControlBlock2 = m_DMA.CreateDMAControlBlock();
  // m_pDMAControlBlock3 = m_DMA.CreateDMAControlBlock();
  // m_pDMAControlBlock4 = m_DMA.CreateDMAControlBlock();

  m_nScreenWidth = ZX_SCREEN_PIXEL_WIDTH;
  m_nScreenHeight = ZX_SCREEN_PIXEL_HEIGHT;
  m_nBorderWidth = (m_nInitWidth - m_nScreenWidth) / 2;
  m_nBorderHeight = (m_nInitHeight - m_nScreenHeight) / 2;

  // Create the framebuffer
  m_pFrameBuffer = new CBcmFrameBuffer (m_nInitWidth, m_nInitHeight, DEPTH,
						      0, 0, m_nDisplay, TRUE);

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

  // Get screen / buffer size   
  m_nWidth  = m_pFrameBuffer->GetWidth ();
  m_nHeight = m_pFrameBuffer->GetHeight ();
  m_nPitch  = m_pFrameBuffer->GetPitch ();
  m_nScreenBufferSize   = m_pFrameBuffer->GetSize (); 
  m_nOffscreenBufferSize = m_nScreenBufferSize / 2;
  m_nPixelCount = m_nOffscreenBufferSize / sizeof(TScreenColor);

  // Sanity check pixel count
  if (m_nPixelCount != m_nWidth * m_nHeight) {
    return FALSE;
  }

  // Ensure that each row is word-aligned so that we can safely use memcpyblk()
  if (m_nPitch % sizeof (u32) != 0)
  {
    return FALSE;
  }
  m_nPitch /= sizeof (TScreenColor);

  // Get pointer to actual buffer - this buffer size varies depending on the colour depth
  m_pScreenBuffer = (TScreenColor *) (uintptr) m_pFrameBuffer->GetBuffer ();

  // Create an offscreen buffer that is half the actual screen buffer (which is double the size of the screen)
  m_pOffscreenBuffer = (TScreenColor *) new u8[m_nOffscreenBufferSize];

  // m_pOffscreenBuffer2 = (TScreenColor *) new u8[m_nOffscreenBufferSize];

  // Clear the screen
  Clear(WHITE_COLOR);


  // HACK
  // m_DMA.SetupMemCopy ((void *)m_pScreenBuffer, (void *)m_pOffscreenBuffer, m_nOffscreenBufferSize, ZX_SCREEN_DMA_BURST_COUNT, FALSE);


  // PeripheralEntry ();
  
  // PeripheralExit ();

	return TRUE;
}

void CZxScreen::Start()
{
  LOGDBG("Starting ZX SCREEN");
  LOGDBG("Width: %dpx, Height: %dpx, Pitch: %d, PixelCount: %d, Screen buffer: %d bytes, Offscreen buffer: %d bytes",
   m_nWidth, m_nHeight, m_nPitch, m_nPixelCount, m_nScreenBufferSize, m_nOffscreenBufferSize);


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
	TScreenColor *pBuffer = m_pOffscreenBuffer;
  // TScreenColor *pBuffer = m_pOffscreenBuffer2;  
	unsigned nSize = m_nPixelCount;
	
	while (nSize--)
	{
		*pBuffer++ = backgroundColor;
	}

  // Mark dirty so will be updated
  m_bDirty = TRUE;
}

void CZxScreen::SetBorder (TScreenColor borderColor)
{	
	TScreenColor *pBuffer = m_pOffscreenBuffer;
	unsigned nSize = m_nPixelCount;
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

  // Mark dirty so will be updated
  m_bDirty = TRUE;
}

// TODO - double buffering, and then DMA
void CZxScreen::SetScreen (boolean bToggle)
{
	TScreenColor *pBuffer = m_pOffscreenBuffer;
	unsigned nSize = m_nPixelCount;
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


  // Mark dirty so will be updated
  m_bDirty = TRUE;
}



void CZxScreen::SetScreenFromBuffer(u16 *pPixelBuffer, u16 *pAttrBuffer, u32 len) {
  TScreenColor *pBuffer = m_pOffscreenBuffer;
	unsigned nSize = m_nPixelCount;
  unsigned x = 0;
  unsigned y = 0;
  unsigned px = 0;
  unsigned py = 0;
  unsigned pixel = 0;
  unsigned pixelWord = 0;
  unsigned pixelBit = 0;
  unsigned pixelSet = 0;
  unsigned attr = 0;
  unsigned pixelColor = 0;
  boolean bInScreen = FALSE;

	while (nSize--)
	{
    px = x - m_nBorderWidth;
    py = y - m_nBorderHeight;
    bInScreen = (px >= 0 && px < (m_nScreenWidth)) && 
                (py >= 0 && py < (m_nScreenHeight));

    // In the screen
    if (bInScreen) { 
      pixelWord = pixel / 8;
      pixelBit = (pixel % 8);
      pixelSet = (pPixelBuffer[pixelWord] >> pixelBit) & 0x01;
      attr = pAttrBuffer[pixelWord];
      pixelColor = getPixelColor(attr, pixelSet);

      // *pBuffer = pixelSet ? BLACK_COLOR : WHITE_COLOR;
      *pBuffer = pixelColor;


      pixel++;
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


  // Mark dirty so will be updated
  m_bDirty = TRUE;
}


//
// Private
//

void CZxScreen::UpdateScreen() {
  if (!m_bDirty) return;

  TScreenColor *pScreenBuffer = m_pScreenBuffer;

  if (m_nScreenBufferNo > 0) {
    pScreenBuffer += m_nPixelCount;
  } 


  // Copy offscreen screen to framebuffer
#ifdef ZX_SCREEN_DMA
    // Have to ensure memory has been written out of the cache to memory before DMA'ing
    CleanAndInvalidateDataCacheRange((uintptr)m_pOffscreenBuffer, m_nOffscreenBufferSize);    

    m_DMA.SetupMemCopy (pScreenBuffer, m_pOffscreenBuffer, m_nOffscreenBufferSize, ZX_SCREEN_DMA_BURST_COUNT, FALSE);

    m_DMA.Start ();
    m_DMA.Wait ();

    // CleanAndInvalidateDataCacheRange((uintptr)m_pOffscreenBuffer, m_nOffscreenBufferSize);
    // CleanAndInvalidateDataCacheRange((uintptr)m_pOffscreenBuffer2, m_nOffscreenBufferSize);

    // m_pDMAControlBlock->SetupMemCopy (pScreenBuffer, m_pOffscreenBuffer, m_nOffscreenBufferSize, ZX_SCREEN_DMA_BURST_COUNT, FALSE);
    // m_pDMAControlBlock->SetWaitCycles(32);
    // m_pDMAControlBlock2->SetupMemCopy (pScreenBuffer, m_pOffscreenBuffer2, m_nOffscreenBufferSize, ZX_SCREEN_DMA_BURST_COUNT, FALSE);
    // m_pDMAControlBlock2->SetWaitCycles(32);
    
    // // Chain
    // m_pDMAControlBlock->SetNextControlBlock (m_pDMAControlBlock2);
    // m_pDMAControlBlock2->SetNextControlBlock (m_pDMAControlBlock);

    // // Ensure CBs are not cached (Start should do this! ..and it does do for this first one, but not chained ones)
    // TDMAControlBlock *pCB1 = m_pDMAControlBlock->GetRawControlBlock();
    // TDMAControlBlock *pCB2 = m_pDMAControlBlock2->GetRawControlBlock();
    // CleanAndInvalidateDataCacheRange ((uintptr) pCB1, sizeof *pCB1); // Ensure cb not cached
    // CleanAndInvalidateDataCacheRange ((uintptr) pCB2, sizeof *pCB2); // Ensure cb not cached

    // m_DMA.Start (m_pDMAControlBlock);
    // m_DMA.Wait ();
#else    
  memcpy(pScreenBuffer, m_pOffscreenBuffer, m_nOffscreenBufferSize);
#endif  


  // Move the virtual screen to the newly written screen
  m_pFrameBuffer->SetVirtualOffset(0, m_nHeight * m_nScreenBufferNo);

  // Increment so next update will update the other part of the framebuffer
  m_nScreenBufferNo = (m_nScreenBufferNo + 1) % SCREEN_BUFFER_COUNT;

  // Mark dirty so will be updated
  m_bDirty = FALSE;
}

TScreenColor CZxScreen::getPixelColor(u8 attr, bool set) {
  u8 colour = 0;
  if (set) {
    colour = attr & 0x07;
  } else {
    colour = (attr >> 3) & 0x07;
  }
  bool bright = attr & 0x40;
  bool flash = attr & 0x80;

  switch (colour) {
    case 0: return bright ? BLACK_COLOR : BLACK_COLOR;
    case 1: return bright ? BRIGHT_BLUE_COLOR : BLUE_COLOR;
    case 2: return bright ? BRIGHT_RED_COLOR : RED_COLOR;
    case 3: return bright ? BRIGHT_MAGENTA_COLOR : MAGENTA_COLOR;
    case 4: return bright ? BRIGHT_GREEN_COLOR : GREEN_COLOR;
    case 5: return bright ? BRIGHT_CYAN_COLOR : CYAN_COLOR;
    case 6: return bright ? BRIGHT_YELLOW_COLOR : YELLOW_COLOR;
    case 7: return bright ? BRIGHT_WHITE_COLOR : WHITE_COLOR;
  }

  return BLACK_COLOR;
}


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





