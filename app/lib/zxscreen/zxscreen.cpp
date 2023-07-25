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

#define ZX_SCREEN_WIDESCREEN    TRUE
// #define ZX_SCREEN_WIDESCREEN    FALSE
#define SCREEN_BUFFER_COUNT 2
// #define SPINLOCK_LEVEL      IRQ_LEVEL /* TASK_LEVEL*/
#define SPINLOCK_LEVEL      TASK_LEVEL  // UNUSED?

#define WIDESCREEN_WIDTH_RATIO  (64 / 45) // 5:4 => 16:9 ((5*9) / (4*16))




u32 borderValue;
u32 videoByteCount;
u32 videoValue;


// Reverse bits (TODO - move to util)

//Index 1==0b0001 => 0b1000
//Index 7==0b0111 => 0b1110
//etc
static unsigned char lookup[16] = {
0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf, };

u8 reverseBits(u8 n) {
   // Reverse the top and bottom nibble then swap them.
   return (lookup[n&0b1111] << 4) | lookup[n>>4];
}

// Detailed breakdown of the math
//  + lookup reverse of bottom nibble
//  |       + grab bottom nibble
//  |       |        + move bottom result into top nibble
//  |       |        |     + combine the bottom and top results 
//  |       |        |     | + lookup reverse of top nibble
//  |       |        |     | |       + grab top nibble
//  V       V        V     V V       V
// (lookup[n&0b1111] << 4) | lookup[n>>4]



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
  m_nZxScreenPixelWidth (0),
	m_nZxScreenPixelHeight (0),
	m_nZxScreenBorderWidth (0),
	m_nZxScreenBorderHeight (0),
	m_nZxScreenWidth (0),
	m_nZxScreenHeight (0),
  m_pScreenBuffer (0),	
  m_pOffscreenBuffer (0),
  // m_pOffscreenBuffer2 (0),
  m_nScreenBufferSize (0),
  m_nOffscreenBufferSize (0),
  m_nZxScreenBufferSize (0),
  m_nPixelCount (0),
  m_nZxScreenPixelCount (0),
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

  // ZX Screen
  m_nZxScreenPixelWidth = ZX_SCREEN_PIXEL_WIDTH;
	m_nZxScreenPixelHeight = ZX_SCREEN_PIXEL_HEIGHT;
	m_nZxScreenBorderWidth = ZX_SCREEN_BORDER_WIDTH;
	m_nZxScreenBorderHeight = ZX_SCREEN_BORDER_HEIGHT;
	m_nZxScreenWidth = ZX_SCREEN_WIDTH;
	m_nZxScreenHeight = ZX_SCREEN_HEIGHT;
  m_nZxScreenBufferSize = m_nZxScreenWidth * m_nZxScreenHeight * sizeof(TScreenColor);
  m_nZxScreenPixelCount = m_nZxScreenBufferSize / sizeof(TScreenColor);

  // Create a ZX screen buffer (which has ZX screen dimensions, which may be to the visible screen)
  m_pZxScreenBuffer = (TScreenColor *) new u8[m_nZxScreenBufferSize];


  // Initialise the zx color LUT
  InitZxColorLUT();

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
  LOGDBG("ZxScreen: Width: %dpx, Height: %dpx, PixelCount: %d, ZxScreen buffer: %d bytes",
   m_nZxScreenWidth, m_nZxScreenHeight, m_nZxScreenPixelCount, m_nZxScreenBufferSize);
  LOGDBG("Screen: Width: %dpx, Height: %dpx, Pitch: %d, PixelCount: %d, Screen buffer: %d bytes, Offscreen buffer: %d bytes",
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
	TScreenColor *pBuffer = m_pZxScreenBuffer;
	unsigned nSize = m_nZxScreenPixelCount;
  unsigned x = 0;
  unsigned y = 0;
	
	while (nSize--)
	{
    // TScreenColor color = WHITE_COLOR;

    if (x < m_nZxScreenBorderWidth || x >= m_nZxScreenWidth - m_nZxScreenBorderWidth || 
        y < m_nZxScreenBorderHeight || y >= m_nZxScreenHeight - m_nZxScreenBorderHeight) {
      // Set pixel to colour
      *pBuffer = borderColor;
    }

    // Increment pixel pointer
    pBuffer++;

    // Track pixel position in buffer
    x++;
    if (x >= m_nZxScreenWidth) {
      x = 0;
      y++;
      if (y >= m_nZxScreenHeight) {
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
	TScreenColor *pBuffer = m_pZxScreenBuffer;
	unsigned nSize = m_nZxScreenPixelCount;
  unsigned x = 0;
  unsigned y = 0;
  unsigned px = 0;
  unsigned py = 0;
  boolean bInScreen = FALSE;

	while (nSize--)
	{
    px = x - m_nZxScreenBorderWidth;
    py = y - m_nZxScreenBorderHeight;
    bInScreen = (px >= 0 && px < (m_nZxScreenWidth)) && 
                (py >= 0 && py < (m_nZxScreenHeight));

    // In the screen
    if (bInScreen) { 
      *pBuffer = bToggle ? rand_32() & 0xFFFF : WHITE_COLOR;
    }

    // Increment pixel pointer
    pBuffer++;

    // Track pixel position in buffer
    x++;
    if (x >= m_nZxScreenWidth) {
      x = 0;
      y++;
      if (y >= m_nZxScreenHeight) {
        y = 0;
      }
    }
	}


  // Mark dirty so will be updated
  m_bDirty = TRUE;
}


void CZxScreen::SetScreenFromULABuffer(u32 frameNo, u16 *pULABuffer, size_t len) {
  // Update offscreen buffer
  ULADataToScreen(frameNo, m_pZxScreenBuffer, pULABuffer, len);

  // Mark dirty so will be updated
  m_bDirty = TRUE;
}


//
// Private
//

void CZxScreen::UpdateScreen() {
  if (!m_bDirty) return;

  TScreenColor *pZxScreenBuffer = m_pZxScreenBuffer;
  TScreenColor *pScreenBuffer = m_pScreenBuffer;

  if (m_nScreenBufferNo > 0) {
    pScreenBuffer += m_nPixelCount;
  } 

  // TODO - Post-process, scale and filter
  // for (unsigned x=0; x<m_nWidth; x++) {
  //   for (unsigned y=0; y<m_nHeight; y++) {
  //     unsigned idx = (y * m_nWidth) + x;
  //     unsigned zxx = ((unsigned)x / (1.4));
  //     unsigned zxy = ((unsigned)y / 1);      
  //     unsigned zxIdx = (zxy * m_nZxScreenWidth) + zxx;
  //     m_pOffscreenBuffer[idx] = pZxScreenBuffer[zxIdx]; // * rand_32();
  //   } 
  // }
  memcpy(m_pOffscreenBuffer, pZxScreenBuffer, m_nZxScreenBufferSize);


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

  // Mark non-dirty so will not be updated unless dirty again
  m_bDirty = FALSE;
}

void CZxScreen::InitZxColorLUT(void) {
  // Init ZX colour LUT - TODO move to function
  for (u32 attr = 0; attr < ZX_COLOR_LUT_SIZE; attr++) {
    u32 ink = 0;
    u32 paper = 0;
    u32 bright = attr & 0x40;
    u32 flash = attr & 0x80;

    for (u32 i = 0; i < 2; i++) {
      u32 colourIn = 0;
      u32 colourOut = 0;
      if (i == 0) {
        // ink
        colourIn = attr & 0x07;
      } else {
        // paper
        colourIn = (attr >> 3) & 0x07;
      }

      switch (colourIn) {
        case 0: default: colourOut = bright ? BLACK_COLOR : BLACK_COLOR; break;
        case 1: colourOut = bright ? BRIGHT_BLUE_COLOR : BLUE_COLOR; break;
        case 2: colourOut = bright ? BRIGHT_RED_COLOR : RED_COLOR; break;
        case 3: colourOut = bright ? BRIGHT_MAGENTA_COLOR : MAGENTA_COLOR; break;
        case 4: colourOut = bright ? BRIGHT_GREEN_COLOR : GREEN_COLOR; break;
        case 5: colourOut = bright ? BRIGHT_CYAN_COLOR : CYAN_COLOR; break;
        case 6: colourOut = bright ? BRIGHT_YELLOW_COLOR : YELLOW_COLOR; break;
        case 7: colourOut = bright ? BRIGHT_WHITE_COLOR : WHITE_COLOR; break;
      }

      if (i == 0) {
        // ink
        ink = colourOut;
      } else {
        // paper
        paper = colourOut;
      }
    }

    ZX_COLORS *pColor = &zxColors[attr];
    pColor->ink = ink;
    pColor->paper = paper;
    pColor->flash = flash;
    pColor->bright = bright;
	}	
}

ZX_COLORS CZxScreen::ZxAttrToScreenColors(u32 attr) {
  return zxColors[(u8)attr];
}

TScreenColor CZxScreen::ZxColorToScreenColor(u32 color) {
  return zxColors[(u8)color].ink;
}

TScreenColor CZxScreen::ZxColorInkToScreenColor(u32 ink) {
  return zxColors[(u8)ink].paper;
}

TScreenColor CZxScreen::ZxColorPaperToScreenColor(u32 paper) {
  return zxColors[(u8)paper].ink;
}


// TODO: Pass in all the sizes
void CZxScreen::ULADataToScreen(u32 frameNo, TScreenColor *pZxScreenBuffer, u16 *pULABuffer, size_t nULABufferLen) {
  // Reset state before processing data (this interrupt occurs at start of screen refresh)
  u32 lastCasValue = 0;
  u32 lastIOWRValue = 0;
  u32 inIOWR = 0;
  u32 inCAS = 0;
  u32 isPixels = TRUE;
  u32 wasOutOfCAS = FALSE;
  videoByteCount = 0;
  u32 pixelData = 0;
  u32 attrData = 0;  
  // u32 nBorderTime;
  u32 nBorderTimeMultiplier = 1;
  u32 inBorder = TRUE;
  u32 inTopBottomBorder = TRUE;
  u32 pixelAttrDataValid = FALSE;  
  u32 borderDrawComplete = FALSE;
  u32 pixelDrawComplete = FALSE;
  
  
  u32 bx = 0; // border x
  u32 by = 0; // border y
  u32 px = m_nZxScreenBorderWidth; // pixel x
  u32 py = m_nZxScreenBorderHeight; // pixel y
  u32 nEndOfZxPixelsY = m_nZxScreenHeight - m_nZxScreenBorderHeight;
  u32 nEndOfZxPixelsX = m_nZxScreenWidth - m_nZxScreenBorderWidth;
  u32 IORQ_MASK = (1 << 2);
  u32 WR_MASK = (1 << 0);
  u32 CAS_MASK = (1 << 1);

  // Update the flash state (on/off every 16 frames)
  u32 flashState = ((frameNo / ZX_SCREEN_FLASH_FRAMES) & 0x01) == 0 ? FALSE : TRUE;

  // Loop all the data in the buffer
  for (unsigned i = 0; i < nULABufferLen; i++) {   
    // Read the value from the ULA data buffer
    // u16 value = pULABuffer[i];
    u32 value = pULABuffer[i];

    // SD2    (MOSI,   PIN19) => /IORQ - When low, indicates an IO read or write - 3S
    // SD1    (MISO,   PIN21) => CAS - Used to detect writes to video memory - 3S
    // SD0    (CE0,    PIN24) => /WR - When low, indicates a write - 3S
    // SOE/SE (GPIO6,  PIN31) <= SMI CLK - Clock for SMI, Switch off except when debugging
    // NOTE: Currently if a frame is dropped, it will glitch the next frame - this should be fixed.

    // Extract the state of the signals
    u32 IORQ = (value & IORQ_MASK) == 0;
    u32 WR = (value & WR_MASK) == 0;
    u32 CAS = (value & CAS_MASK) == 0;           
    u32 IOREQ = IORQ;
  
    // Handle ULA video read (/CAS in close succession (~100ns in-between each cas)
    
    // At start of frame, de-glitching is necssary to ensure the first CAS detected is not a glitch
    // This is achieved by using the INT signal as part of the DREQ, and discarding the first CAS
    if (i > 50) {
      if (!wasOutOfCAS) {            
        wasOutOfCAS = !CAS;      
      }

      if (wasOutOfCAS && CAS) {
        // Entered or in CAS
        lastCasValue = value;
        inCAS++;
      } else {
        if (inCAS >= 3 && inCAS <= 5) {
        // // pThis->m_value = lastValue;   
          if (videoByteCount < 0x3000) {
            videoByteCount++;                

            if (isPixels) {              
              pixelData = lastCasValue >> 8;    
            } else {
              attrData = lastCasValue >> 8;                
#if (HD_SPECCYS_BOARD_REV == 1)
              // HW Fix, reverse the 8 bits
              attrData = reverseBits(attrData);
#endif               
              pixelAttrDataValid = TRUE;
            }
            isPixels = !isPixels;
          }
        } 
        inCAS = 0;
      }
    }
    
    // Handle IO write (border colour)
    if (IOREQ & WR) {          
      // Entered or in IO write
      lastIOWRValue = value;
      inIOWR++;          
    } else {
      if (inIOWR > 5 && inIOWR < 40) {
        // Exited IO write, use the last value
        borderValue = lastIOWRValue >> 8;
#if (HD_SPECCYS_BOARD_REV == 1)
        // HW Fix, reverse the 8 bits
        borderValue = reverseBits(borderValue);
#endif        
      }
      inIOWR = 0;
    }

    // if (i == 400) {
    // borderValue = value;
    // }

    // Draw border
    if (!borderDrawComplete) {
      for (u32 i=0; i<nBorderTimeMultiplier; i++) {
        inTopBottomBorder = by < m_nZxScreenBorderHeight || by >= nEndOfZxPixelsY;
        inBorder = (bx < m_nZxScreenBorderWidth || bx >= nEndOfZxPixelsX || inTopBottomBorder);
        // nBorderTimeMultiplier = inTopBottomBorder ? 4 : 1;

        if (inBorder /*&& nBorderTime % 100 == 0*/) {
          // TScreenColor borderColor = getPixelColor(borderValue, 0, 0, 1);
          TScreenColor borderColor = ZxColorToScreenColor(borderValue);
          u32 bytePos = by * m_nZxScreenWidth + bx;
          pZxScreenBuffer[bytePos] = borderColor;
        }

        bx++;
        if (bx >= m_nZxScreenWidth) {
          bx = 0;
          by++;
          if (by >= m_nZxScreenHeight) {
            // End of screen, break out of loop
            borderDrawComplete = TRUE;
            break;
          }
        } 
      }
    }
    // nBorderTime++;



    // Draw pixels
    if (pixelAttrDataValid && !pixelDrawComplete) {
      pixelAttrDataValid = FALSE;

      // Proces the pixel and attribute data
      ZX_COLORS colours = ZxAttrToScreenColors(attrData);
      
      u32 bytePos = py * m_nZxScreenWidth + px;

      for (int i = 0; i < 8; i++) {
        // 7-i to flip the data lines
#if (HD_SPECCYS_BOARD_REV == 1)
        bool pixelSet = pixelData >> i & 0x01;
#else        
        bool pixelSet = pixelData << i & 0x80;
#endif      
        // Flip the pixel state if flash and in flash state
        if (colours.flash && flashState) {
          pixelSet = !pixelSet;
        }
#if ZX_SCREEN_COLOUR     
        TScreenColor pixelColor = pixelSet ? colours.ink : colours.paper;
        pZxScreenBuffer[bytePos + i] = pixelColor;
#else
        pZxScreenBuffer[bytePos + i] = pixelSet ? BLACK_COLOR : WHITE_COLOR;
#endif        
      }

      // TODO - should try to standardise all buffers and buffer pointers to u32 as it is MUCH faster than
      // operating on u16 or u8 - this should be possible because the SMI data is 16 bit attr (+control)
      // and 16 bit pixel data (+control)

      // Move to the next pixels (byte)
      px += 8;
      if (px >= (m_nZxScreenWidth - m_nZxScreenBorderWidth)) {
        px = m_nZxScreenBorderWidth;
        py++;
        if (py >= m_nZxScreenHeight - m_nZxScreenBorderHeight) {
          // End of screen, break out of loop
          pixelDrawComplete = TRUE;
        }
      }      
    }

    // Break out of loop if screen is drawn
    if (borderDrawComplete && pixelDrawComplete) break;
  }
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





