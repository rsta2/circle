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
#define SPINLOCK_LEVEL      TASK_LEVEL  // UNUSED?

// TODO - these are potentially HW dependent and should be in config.h
#define IORQ_MASK (1 << 2)
#define WR_MASK   (1 << 0)
#define CAS_MASK  (1 << 1)


u32 borderValue;
u32 videoByteCount;
u32 videoValue;
u32 CAS_LENGTHS[30];


inline void processUlaCas(
  bool CAS, 
  u32 dataValue,
  u32 &inCAS, 
  u32 &ulaCASCount,     
  u32 *pPixelAttrData, 
  unsigned pZxScreenData[],
  u32 &videoByteCount,
  u32 &pixelDataComplete
);

inline void processUlaIoWrite(
  bool ulaWR, 
  u32 dataValue,
  u32 &inUlaWR, 
  u32 &borderValue,     
  u32 &borderChangeIndex,     
  u32 *pBorderTimingBuffer, 
  size_t nBorderTimingBufferLen,
  unsigned pZxBorderData[],
  u32 &borderDataComplete
);

// Reverse bits (TODO - move to util)

//Index 1==0b0001 => 0b1000
//Index 7==0b0111 => 0b1110
//etc
static unsigned char lookup[16] = {
0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf, };

inline u8 reverseBits(u8 n) {
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
  // Start with a white border
  borderValue = 0x07;

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

  delete m_pZxScreenData;
  delete m_pZxBorderData;
}

boolean CZxScreen::Initialize ()
{
  LOGNOTE("Initializing ZX SCREEN");

  // Create the framebuffer
  m_pFrameBuffer = new CBcmFrameBuffer (m_nInitWidth, m_nInitHeight, ZX_SCREEN_DEPTH,
						      0, 0, m_nDisplay, TRUE);

  // Initialize the framebuffer
  if (!m_pFrameBuffer->Initialize ())
  {
    return FALSE;
  }

  // Check depth is correct
  if (m_pFrameBuffer->GetDepth () != ZX_SCREEN_DEPTH)
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

  // Create a ZX screen data buffer (which has data in the internal ZX screen format, but u32 for efficient processing)
  m_pZxScreenData = new unsigned[m_nZxScreenBufferSize];

  // Create a ZX border data buffer ([0] = border colour, [1] = border byte change, ...repeats)
  m_pZxBorderData = new unsigned[ZX_SCREEN_BORDER_DATA_LENGTH];


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


void CZxScreen::SetScreenFromULABuffer(u32 frameNo, u16 *pULABuffer, size_t nULABufferlen, u32 *pBorderTimingBuffer, size_t nBorderTimingBufferLen) {
  // Update offscreen buffer
  ULADataToScreen(frameNo, pULABuffer, nULABufferlen, pBorderTimingBuffer, nBorderTimingBufferLen);
  // ULADataToScreen_OLD(frameNo, pULABuffer, nULABufferlen, pBorderTimingBuffer, nBorderTimingBufferLen);

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
  for (u32 attr = 0; attr < ZX_SCREEN_COLOR_LUT_SIZE; attr++) {
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

// TODO: Move all this to a ZxUlaToScreen class
// TODO: Pass in all the sizes
// TODO: There is a fundamental problem that A0 is not considered for border writes, that means IO writes to external
// peripherals will cause the border to incorrectly change colour.
// NOTE: Stability was found by ensuring that the 4 ULA /CAS low pulses are detected sequentially, rather than
// just trying to detect individual /CAS low pulses. This makes the code more resistant against sample glitches
// but I am not 100% sure why it works, and considering the pulses singley did not.
void CZxScreen::ULADataToScreen(
  u32 frameNo,
  u16 *pULABuffer, 
  size_t nULABufferLen, 
  u32 *pBorderTimingBuffer, 
  size_t nBorderTimingBufferLen
) {
  // Reset state before processing data (this interrupt occurs at start of screen refresh)
  // TODO: remove unused values!
  u32 dataValue = 0;
  u32 value = 0;
  u32 CAS = 0;
  u32 IORQ = 0;
  u32 WR = 0; 
  u32 IOREQ = 0;
  u32 ulaWR = 0;
  u32 inCAS = 0;
  u32 inUlaWR = 0;
  u32 ulaCASCount = 0;
  u32 pPixelAttrData[4];  
  videoByteCount = 0; // Exported for debug reasons
  u32 borderChangeIndex = 0;
  u32 borderDataComplete = FALSE;
  u32 pixelDataComplete = FALSE;
  
  
  // For debugging only
  for (unsigned i = 0; i < 30; i++) {
    CAS_LENGTHS[i] = 0;
  }

  // Loop all the data in the buffer
  unsigned i = 0;
  while (i < nULABufferLen) {   
    // NOTE: Currently if a frame is dropped, it will glitch the next frame - this should be fixed.

    // Find the next valid ULA screen read (4 short CAS pulses)        
    
    // Data value is latched on sample before the deactivation of the relevant signals
    dataValue = value;  

    // Get the next sample from the ULA buffer
    value = pULABuffer[i++];

    // Optimization: no need to process CAS at all if the screen is drawn
    // CAS will be inactive when pixelDataComplete is set true, so border optimisation below will work
    if (!pixelDataComplete) {
      CAS = (value & CAS_MASK) == 0;  

      // Process the ULA CAS (pixel and attribute data)
      processUlaCas(
        CAS,      
        dataValue, 
        inCAS, 
        ulaCASCount,           
        pPixelAttrData, 
        m_pZxScreenData,
        videoByteCount,
        pixelDataComplete
      );
    }
    
   
    // Process ULA IO write (border colour capture)
    // NOTE: There is a special case where there were NO border writes in the frame
    if (nBorderTimingBufferLen > 0) {
      // Optimisations: 
      // - IO signal states while CAS is active are not relevant as they don't change
      // - No need to process IO writes at all if the border data is complete

      if (!borderDataComplete && !CAS) { 
        IORQ = (value & IORQ_MASK) == 0;
        WR = (value & WR_MASK) == 0;
        IOREQ = IORQ; // NOTE: This is a bug. A0 must be considered but is not wired in HW. Peripheral IO writes will cause border colour to change.
        ulaWR = IOREQ & WR;

        processUlaIoWrite(
          ulaWR,
          dataValue,
          inUlaWR,
          borderValue, // TODO: currently global - preserved between frames
          borderChangeIndex,
          pBorderTimingBuffer,
          nBorderTimingBufferLen,
          m_pZxBorderData,
          borderDataComplete
        );
      }
    } else {
      // No border writes in this frame.
      borderDataComplete = TRUE;
    }
    // borderDataComplete = TRUE;
  

    // Break out of loop if screen is drawn (as the rest of the data is not needed)
    if (pixelDataComplete && borderDataComplete) break;

  } // while (i < nULABufferLen)

  // Special border case when there is no border written this frame
  if (nBorderTimingBufferLen == 0) {
      // No border writes in this frame. Fill the border data with the current border colour  
      for (u32 i = 0; i < ZX_SCREEN_BORDER_DATA_LENGTH; i++) m_pZxBorderData[i] = borderValue;
  }

  // NOTE: It is more efficient to write the border across the entire screen, then the screen pixels over the top
  // than to calculate and only write the border pixels.

  // Draw the border data to the screen (??ms)
  CopyZxBorderDataToZxScreenBuffer(frameNo);

  // Draw the screen data to the screen (0.5ms)
  CopyZxScreenDataToZxScreenBuffer(frameNo);
}

inline void processUlaCas(
  bool CAS,   
  u32 dataValue,
  u32 &inCAS, 
  u32 &ulaCASCount,       
  u32 *pPixelAttrData, 
  unsigned pZxScreenData[],
  u32 &videoByteCount,
  u32 &pixelDataComplete
) {
  if (CAS) {
    inCAS++;
  } else {

    // For debugging only
    if (inCAS > 0 && inCAS < 30) {
      CAS_LENGTHS[inCAS]++;
    }

    // Process ULA CAS ('short' CAS, video memory read for display)
    if (inCAS > 3 && inCAS <= 5) {

      pPixelAttrData[ulaCASCount] = dataValue >> 8;
#if (ZX_SCREEN_REVERSE_DATA_BITS)
      // HW Fix, reverse the 8 bits of the attribute value (pixels are reversed later)
      if (ulaCASCount & 0x01) {
        pPixelAttrData[ulaCASCount] = reverseBits(pPixelAttrData[ulaCASCount]);
      }
#endif          
      ulaCASCount++;     

      if (ulaCASCount == 4) {
        // Exited the 4th ULA CAS, so have a complete pixel and attribute data x2
        memcpy(&pZxScreenData[videoByteCount], pPixelAttrData, sizeof(u32) * 4);
        videoByteCount += 4;

        if (videoByteCount >= 0x3000) {
          // End of screen
          pixelDataComplete = TRUE;
        }
        
        ulaCASCount = 0;  // Reset CAS count as hit 4th ULA CAS    
      }
    } else if (inCAS > 5) {
      // Non-ULA 'long' CAS from Z80, reset ULA CAS count
      ulaCASCount = 0;             
    } else {
      // CAS pulse shorter than 3 - this is a glitch and must be ignored
    }
    inCAS = 0;
  }
}

inline void processUlaIoWrite(
  bool ulaWR, 
  u32 dataValue,
  u32 &inUlaWR, 
  u32 &borderValue,     
  u32 &borderChangeIndex,     
  u32 *pBorderTimingBuffer, 
  size_t nBorderTimingBufferLen,
  unsigned pZxBorderData[],
  u32 &borderDataComplete
) {
  if (ulaWR) {    
    inUlaWR++;
  } else {
    if (inUlaWR > 5) {
      // A border write has been performed. 
      // Set the data used to write the border up to this write
      // If this is the last write, then complete the drawing of the border
      if (borderChangeIndex < nBorderTimingBufferLen) { 
        
        // Get the time difference between this write and the previous (0 if first write)
        u32 borderWriteTimePrevUs = 0;
        if (borderChangeIndex > 0) borderWriteTimePrevUs = pBorderTimingBuffer[borderChangeIndex - 1];
        u32 borderWriteTimeUs = pBorderTimingBuffer[borderChangeIndex];

        // Convert the times to pixel 'byte' counts (since the border is written in 8 pixel blocks)
        // TODO: It might be better to round these values, rather than truncate
        u32 pixelByteCountPrev = borderWriteTimePrevUs / ZX_BORDER_TIME_TO_PIXEL_RATIO;
        u32 pixelByteCount = borderWriteTimeUs / ZX_BORDER_TIME_TO_PIXEL_RATIO;        
        if (false) {
          u32 divisorMinus1 = ZX_BORDER_TIME_TO_PIXEL_RATIO - 1;
          pixelByteCountPrev = (borderWriteTimePrevUs + divisorMinus1)  / ZX_BORDER_TIME_TO_PIXEL_RATIO;
          pixelByteCount = (borderWriteTimeUs + divisorMinus1) / ZX_BORDER_TIME_TO_PIXEL_RATIO;
        }

        // Fill the border data between the previous border colour write and this one with the previous colour
        u32 idxPrev = pixelByteCountPrev;
        u32 idx = min(pixelByteCount, ZX_SCREEN_BORDER_DATA_LENGTH);
        for (u32 i = idxPrev; i < idx; i++) {
          pZxBorderData[i] = borderValue;
        }

        // Set the border value for this write
        borderValue = dataValue >> 8;
#if (ZX_SCREEN_REVERSE_DATA_BITS)
        // HW Fix, reverse the 8 bits
        borderValue = reverseBits(borderValue);
#endif     
        
        // Increment the border change index
        borderChangeIndex++;

        // Check if this was the final border write this frame
        borderDataComplete = borderChangeIndex >= nBorderTimingBufferLen;
          
        if (borderDataComplete) {
          // Reached the end of the border timing info. Draw the final part of the border
          for (u32 i = idx; i < ZX_SCREEN_BORDER_DATA_LENGTH; i++) {
            pZxBorderData[i] = borderValue;
          }
        }
      } else {
        // Should not happen - more border writes than border timing data - ignore, she'll be right.
      }

    } else {
      // IOWR pulse shorter than 5 - this is a glitch and must be ignored
    }
    inUlaWR = 0;
  }  
}

void CZxScreen::CopyZxScreenDataToZxScreenBuffer(u32 frameNo) {
  unsigned *pZxScreenData = m_pZxScreenData;
  TScreenColor *pZxScreenBuffer = m_pZxScreenBuffer;

  // Update the flash state (on/off every 16 frames)
  u32 flashState = ((frameNo / ZX_SCREEN_FLASH_FRAMES) & 0x01) == 0 ? FALSE : TRUE;

  u32 px = m_nZxScreenBorderWidth; // pixel x
  u32 py = m_nZxScreenBorderHeight; // pixel y


  for (unsigned i = 0; i < ZX_SCREEN_DATA_LENGTH; i += 2) {   
    u32 pixelData = pZxScreenData[i];
    u32 attrData = pZxScreenData[i+1];

    // Proces the pixel and attribute data
    ZX_COLORS colours = ZxAttrToScreenColors(attrData);
    
    u32 bytePos = py * m_nZxScreenWidth + px;

    for (int i = 0; i < 8; i++) {
      // 7-i to flip the data lines
#if (ZX_SCREEN_REVERSE_DATA_BITS)
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
        // End of screen
        break; // Will be end of data anyway
      }
    }      
  }
}

void CZxScreen::CopyZxBorderDataToZxScreenBuffer(u32 frameNo) {  
  TScreenColor borderColor = -1;
  TScreenColor prevBorderColor = -1;  
  u32 prevBorderValue = -1;

  u32 nScreenWidth = m_nZxScreenWidth / 8;  // Width converted to bytes
  u32 nScreenHeight = m_nZxScreenHeight;
  u32 nBorderWidth = m_nZxScreenBorderWidth / 8; // Width converted to bytes
  u32 nBorderHeight = m_nZxScreenBorderHeight;
  u32 nEndOfZxPixelsX = (nScreenWidth - nBorderWidth);  
  u32 nEndOfZxPixelsY = (nScreenHeight - nBorderHeight);  

  // HACK - reset border
  // memset(m_pZxScreenBuffer, 0, m_nZxScreenBufferSize);

  for (u32 i = 0; i < ZX_SCREEN_BORDER_DATA_LENGTH; i++) {

    // Get and convert the borderValue (attribute) to border colour, only converting when colour changes.
    u32 borderValue = m_pZxBorderData[i];
    if (borderValue == prevBorderValue) {
      borderColor = prevBorderColor;
    } else {
      borderColor = ZxColorToScreenColor(borderValue);
      prevBorderColor = borderColor;
      prevBorderValue = borderValue;
    }
    // borderColor = ZxColorToScreenColor(0x33);

    // Convert the index to the screen buffer position
    // The original screen timing of the spectrum is considered, then mapped to the screen buffer
    // The frame interrupt occurs at the start of the screen pixels on the left side (NOT at the start of the
    // television line or left border). However,there will be an offset needed in the code here for the interrupt
    // processing delay, and it is unlikely to be possible to get the border timing perfect as the interrupts will
    // not be microsecond accurate. Try the best we can.
    // 448 / 8 = 56 bytes per line
    // 312 lines per frame
    // 96 / 8 = 12 bytes offset before left border pixels start
    // 32 / 8 = 4 bytes for left border
    // 256 / 8 = 32 bytes for screen pixels    
    // 64 / 8 = 8 bytes for right border (but we'll only display 4!)
    // Total 128 / 8 = 16 bytes offset before screen pixels start
    // 8 lines before top border pixels start
    // 56 lines of top border (but we'll only display 32!)
    // 192 lines of screen pixels
    // 56 lines of bottom border (but we'll only display 32!)
    // TODO - define the above as constants

    u32 idx = i + 16; // Add the 16 byte offset

    // Interrupt processing delay offset (will be PI dependent)
    idx -= 1;

    u32 zxBorderX = idx % 56;
    u32 zxBorderY = idx / 56;


    // Map the ZX border position to the screen buffer XY position
    u32 screenBufferX = (zxBorderX - 12); // 12 (HSYNC)
    u32 screenBufferY = (zxBorderY - 32); // 8 (VSYNC) + 24 (top border off-screen)

    // Check if this byte is in the visible border, otherwise it can be ignored
    u32 xInRange = screenBufferX >= 0 && screenBufferX < nScreenWidth;
    u32 yInRange = screenBufferY >= 0 && screenBufferY < nScreenHeight;
    u32 inVisibleBorder = xInRange && yInRange;
    // u32 inVisibleTopBottomBorder = xInRange &&
    //   ((screenBufferY >= 0 && screenBufferY < nBorderHeight) || 
    //   (screenBufferY >= nEndOfZxPixelsY && screenBufferY < nScreenHeight));  
    // u32 inVisibleSideBorder = 
    //   (screenBufferX >= 0 && screenBufferX < nBorderWidth) || 
    //   (screenBufferX >= nEndOfZxPixelsX && screenBufferX < nScreenWidth);    
    // u32 inVisibleBorder = inVisibleTopBottomBorder || inVisibleSideBorder;


    // Draw the border colour to the screen buffer
    if (inVisibleBorder) {            
      u32 bytePos = screenBufferY * nScreenWidth + screenBufferX;
      u32 bitPos = bytePos * 8;
      if (bitPos >= 0 && bitPos < m_nZxScreenBufferSize) {
        for (u32 bit = 0; bit < 8; bit++) m_pZxScreenBuffer[bitPos + bit] = borderColor;
      }
    }

    // bx++;
    // if (bx >= m_nZxScreenWidth) {
    //   bx = 0;
    //   by++;
    //   if (by >= m_nZxScreenHeight) {
    //     // End of border, break out of loop
    //     break; // Will be end of data anyway
    //   }
    // }     
  }
}


// TODO: Pass in all the sizes
// TODO: There is a fundamental problem that A0 is not considered for border writes, that means IO writes to external
// peripherals will cause the border to incorrectly change colour.
void CZxScreen::ULADataToScreen_OLD(
  u32 frameNo,
  u16 *pULABuffer, 
  size_t nULABufferLen, 
  u32 *pBorderTimingBuffer, 
  size_t nBorderTimingBufferLen
) {
  // Reset state before processing data (this interrupt occurs at start of screen refresh)
  TScreenColor *pZxScreenBuffer = m_pZxScreenBuffer;
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
  u32 inBorder = TRUE;
  u32 inTopBottomBorder = TRUE;
  u32 pixelAttrDataValid = FALSE;  
  u32 borderDrawComplete = FALSE;
  u32 pixelDrawComplete = FALSE;

  u32 nDrawBorderTimingIndex = 0;
  
  u32 bx = 0; // border x
  u32 by = 0; // border y
  u32 px = m_nZxScreenBorderWidth; // pixel x
  u32 py = m_nZxScreenBorderHeight; // pixel y
  u32 nEndOfZxPixelsY = m_nZxScreenHeight - m_nZxScreenBorderHeight;
  u32 nEndOfZxPixelsX = m_nZxScreenWidth - m_nZxScreenBorderWidth;

  // Update the flash state (on/off every 16 frames)
  u32 flashState = ((frameNo / ZX_SCREEN_FLASH_FRAMES) & 0x01) == 0 ? FALSE : TRUE;

  for (unsigned i = 0; i < 30; i++) {
    CAS_LENGTHS[i] = 0;
  }

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

    u32 bDrawBorder = false;
    u32 bDrawBorderFinalPart = false;
    u32 drawBorderValue = 0;
    u32 drawBorderCount = 0;
  
    // Handle ULA video read (/CAS in close succession (~100ns in-between each cas)
    
    // At start of frame, de-glitching is necssary to ensure the first CAS detected is not a glitch
    // This is achieved by using the INT signal as part of the DREQ, and discarding the first CAS

    // TODO - how the data array is processed for CAS needs optimising, as it is currently too CPU intensive and
    // error prone.
    // It should be possible that as soon as we detect a CAS, we can look ahead in the array to quickly decide if
    // it is a video CAS or not. If we can optimise this, we can increase the DMA buffer length and the accuracy
    // of the video data detection.

    if (i > 50) {
      if (!wasOutOfCAS) {            
        wasOutOfCAS = !CAS;      
      }

      if (wasOutOfCAS && CAS) {
        // Entered or in CAS
        lastCasValue = value;
        inCAS++;
      } else {
        if (inCAS > 0 && inCAS < 30) {
          CAS_LENGTHS[inCAS]++;
        }

        if (inCAS >= 3 && inCAS <= 5) {
        // // pThis->m_value = lastValue;   
          if (videoByteCount < 0x3000) {
            videoByteCount++;                

            if (isPixels) {              
              pixelData = lastCasValue >> 8;    
            } else {
              attrData = lastCasValue >> 8;                
#if (ZX_SCREEN_REVERSE_DATA_BITS)
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
      // if (inIOWR > 5 && inIOWR < 40) {
      if (inIOWR > 5 /*&& inIOWR < 40*/) {
      // if (inIOWR > 0) {
        // A border write has been performed. 
        // Set the data used to write the border up to this write
        // If this is the last write, then complete the drawing of the border
        bDrawBorder = true;
        drawBorderValue = borderValue;              

        if (nDrawBorderTimingIndex < nBorderTimingBufferLen) { 
          drawBorderCount = pBorderTimingBuffer[nDrawBorderTimingIndex];
          if (nDrawBorderTimingIndex > 0) {
            drawBorderCount -= pBorderTimingBuffer[nDrawBorderTimingIndex - 1];
          }
          // TODO: Using a hack factor here is too simple because the border timing below only considers on screen
          // time, but there is offscreen time as well. The border loop below needs to be corrected to account for
          // the offscreen time as well.
          drawBorderCount = (u32) drawBorderCount * 4.2;  // HACK factor
          nDrawBorderTimingIndex++;

          if (nDrawBorderTimingIndex >= nBorderTimingBufferLen) {
            // Reached the end of the border timing info. Draw the final part of the border
            bDrawBorderFinalPart = true;
          }
        } else {
          // Should not happen - more border writes than border timing data - just draw the final part of the border 
          // in this case  
          bDrawBorderFinalPart = true;
        }

        // Exited IO write, use the last value
        borderValue = lastIOWRValue >> 8;
#if (ZX_SCREEN_REVERSE_DATA_BITS)
        // HW Fix, reverse the 8 bits
        borderValue = reverseBits(borderValue);
#endif        
      }
      inIOWR = 0;
    }


    // Draw border

    // Handle the special case where there is no border data (just write entire border in current colour)
    // TODO: What if there is border timing data, but no border data in the ULA buffer?
    // In this case, we'll loop the entire ULA buffer looking for the non-existing border data
    // This is unlikely to happen, but needs to be handled somehow (it will be tricky!!)
    if (nBorderTimingBufferLen == 0) {
      bDrawBorder = true;
      bDrawBorderFinalPart = true;
      drawBorderCount = 0;      
      drawBorderValue = borderValue;
    }
    // borderDrawComplete = true;


    if (!borderDrawComplete) {
      if (bDrawBorder) {
        // TScreenColor borderColor = getPixelColor(borderValue, 0, 0, 1);
        TScreenColor borderColor = ZxColorToScreenColor(drawBorderValue);

        for (u32 loops=0; loops<2; loops++) {
          // Loop 1: draw previous border part
          // Loop 2: draw final border part if 
          for (u32 i=0; i<drawBorderCount; i++) {
            inTopBottomBorder = by < m_nZxScreenBorderHeight || by >= nEndOfZxPixelsY;
            inBorder = (bx < m_nZxScreenBorderWidth || bx >= nEndOfZxPixelsX || inTopBottomBorder);
            // nBorderTimeMultiplier = inTopBottomBorder ? 4 : 1;

            if (inBorder /*&& nBorderTime % 100 == 0*/) {            
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
          if (borderDrawComplete || !bDrawBorderFinalPart) break;
          drawBorderValue = borderValue; // Fill final part of border in current colour
          drawBorderCount = m_nZxScreenWidth * m_nZxScreenHeight * 2; // Ensure all remaining pixels are drawn
        }
      }
    }


    // Draw pixels
    if (pixelAttrDataValid && !pixelDrawComplete) {
      pixelAttrDataValid = FALSE;

      // Proces the pixel and attribute data
      ZX_COLORS colours = ZxAttrToScreenColors(attrData);
      
      u32 bytePos = py * m_nZxScreenWidth + px;

      for (int i = 0; i < 8; i++) {
        // 7-i to flip the data lines
#if (ZX_SCREEN_REVERSE_DATA_BITS)
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




