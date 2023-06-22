//
// zxsmi.cpp
//
#include <zxsmi/zxsmi.h>
#include <circle/util.h>

// HACK FOR SMI
#include <circle/memio.h>


LOGMODULE ("ZxSmi");

#define GPIO_IRQ_PIN    27

// HACK - direct access to SMI
#define ARM_SMI_BASE	(ARM_IO_BASE + 0x600000)
#define ARM_SMI_CS		(ARM_SMI_BASE + 0x00)	// Control & status
#define ARM_SMI_L		(ARM_SMI_BASE + 0x04)	// Transfer length/count
#define ARM_SMI_A		(ARM_SMI_BASE + 0x08)	// Address
#define ARM_SMI_D		(ARM_SMI_BASE + 0x0c)	// Data
#define ARM_SMI_DSR0	(ARM_SMI_BASE + 0x10)	// Read settings device 0
#define ARM_SMI_DSW0	(ARM_SMI_BASE + 0x14)	// Write settings device 0
#define ARM_SMI_DSR1	(ARM_SMI_BASE + 0x18)	// Read settings device 1
#define ARM_SMI_DSW1	(ARM_SMI_BASE + 0x1c)	// Write settings device 1
#define ARM_SMI_DSR2	(ARM_SMI_BASE + 0x20)	// Read settings device 2
#define ARM_SMI_DSW2	(ARM_SMI_BASE + 0x24)	// Write settings device 2
#define ARM_SMI_DSR3	(ARM_SMI_BASE + 0x28)	// Read settings device 3
#define ARM_SMI_DSW3	(ARM_SMI_BASE + 0x2c)	// Write settings device 3
#define ARM_SMI_DMC		(ARM_SMI_BASE + 0x30)	// DMA control
#define ARM_SMI_DCS		(ARM_SMI_BASE + 0x34)	// Direct control/status
#define ARM_SMI_DCA		(ARM_SMI_BASE + 0x38)	// Direct address
#define ARM_SMI_DCD		(ARM_SMI_BASE + 0x3c)	// Direct data
#define ARM_SMI_FD		(ARM_SMI_BASE + 0x40)	// FIFO debug

// CS Register
#define CS_ENABLE		(1 << 0)
#define CS_DONE			(1 << 1)
//#define CS_ACTIVE		(1 << 2)	// TODO: name clash with <circle/dmacommon.h>
#define CS_START		(1 << 3)
#define CS_CLEAR		(1 << 4)
#define CS_WRITE		(1 << 5)
//#define CS_UNUSED1__SHIFT	6
#define CS_TEEN			(1 << 8)
#define CS_INTD			(1 << 9)
#define CS_INTT			(1 << 10)
#define CS_INTR			(1 << 11)
#define CS_PVMODE		(1 << 12)
#define CS_SETERR		(1 << 13)
#define CS_PXLDAT		(1 << 14)
#define CS_EDREQ		(1 << 15)
//#define CS_UNUSED2__SHIFT	16
#define CS_PRDY			(1 << 24)
#define CS_AFERR		(1 << 25)
#define CS_TXW			(1 << 26)
#define CS_RXR			(1 << 27)
#define CS_TXD			(1 << 28)
#define CS_RXD			(1 << 29)
#define CS_TXE			(1 << 30)
#define CS_RXF			(1 << 31)


u32 screenLoopTicks = 0;
u32 screenLoopCount = 0;
u32 frameInterruptCount = 0;
u32 skippedFrameCount = 0;

u32 skippy = 0;

//
// Class
//

CZxSmi::CZxSmi (CGPIOManager *pGPIOManager, CInterruptSystem *pInterruptSystem)
: m_pGPIOManager(pGPIOManager),
  m_pInterruptSystem (pInterruptSystem),
#if (ZX_SMI_USE_FIQ)
  m_GpioFiqPin (GPIO_IRQ_PIN, GPIOModeInput, pInterruptSystem),
#else  
  m_GpioIrqPin(GPIO_IRQ_PIN, GPIOModeInput, m_pGPIOManager),
#endif  
  m_SMIMaster (ZX_SMI_DATA_LINES_MASK, ZX_SMI_USE_ADDRESS_LINES, ZX_SMI_USE_SOE_SE, ZX_SMI_USE_SWE_SRW, pInterruptSystem),
  m_DMA (ZX_DMA_CHANNEL_TYPE, m_pInterruptSystem),
	m_pDMAControlBlock(0),
  m_pDMAControlBlock1(0),
  m_pDMAControlBlock2(0),
  m_pDMAControlBlock3(0),
  m_pDMAControlBlock4(0),
	m_pDMABuffer(0),
	m_nDMABufferIdx(0),
  m_nDMABufferLenBytes(0),
	m_nDMABufferLenWords(0),
    m_pDMABufferRead (0),
	m_nDMABufferIdxRead (0),
  m_DMABufferReadSemaphore (1),
  m_pFrameEvent(0),
  m_bRunning(FALSE),  
  m_pActLED(0),
  m_bDMAResult(0),
  m_counter(0),
  m_value(0),
  m_src(0),
  m_isIOWrite(0),
  m_counter2(0),
  m_pDebugBuffer(0)
{
  m_nDMABufferLenBytes = ZX_DMA_BUFFER_LENGTH;
	m_nDMABufferLenWords = ZX_DMA_BUFFER_LENGTH * sizeof(ZX_DMA_T);
}

CZxSmi::~CZxSmi (void)
{  
  m_DMA.FreeDMAControlBlock(m_pDMAControlBlock1);
  m_DMA.FreeDMAControlBlock(m_pDMAControlBlock2);
  m_DMA.FreeDMAControlBlock(m_pDMAControlBlock3);
  m_DMA.FreeDMAControlBlock(m_pDMAControlBlock4);

    // Deallocate DMA buffers and control blocks
  for (unsigned i = 0; i < ZX_DMA_BUFFER_COUNT; i++) {
    // DMA control block
    m_DMA.FreeDMAControlBlock(m_pDMAControlBlocks[i]);

    // Target data buffer
    delete m_pDMABuffers[i];
  }

#if (ZX_SMI_USE_FIQ)
  m_GpioFiqPin.DisableInterrupt ();
#else
  m_GpioIrqPin.DisableInterrupt ();
#endif  
}

boolean CZxSmi::Initialize ()
{
  LOGNOTE("Initializing SMI");

  // PeripheralEntry ();
  
  // Allocate DMA control blocks
  m_pDMAControlBlock1 = m_DMA.CreateDMAControlBlock();
  m_pDMAControlBlock2 = m_DMA.CreateDMAControlBlock();
  m_pDMAControlBlock3 = m_DMA.CreateDMAControlBlock();
  m_pDMAControlBlock4 = m_DMA.CreateDMAControlBlock();

  // Allocate DMA buffers and control blocks
  for (unsigned i = 0; i < ZX_DMA_BUFFER_COUNT; i++) {
    // DMA control block
    m_pDMAControlBlocks[i] = m_DMA.CreateDMAControlBlock();

    // Target data buffer
    m_pDMABuffers[i] = new ZX_DMA_T[m_nDMABufferLenWords]; // using new makes the buffer cache-aligned, so suitable for DMA
  }

  // Debug buffer
  m_pDebugBuffer = new ZX_DMA_T[ZX_SMI_DEBUG_BUFFER_LENGTH];


  // Setup SMI timing
  LOGDBG("Setting SMI timing: width: %d, ns: %d, setup: %d, strobe: %d, hold: %d, pace: %d, external: %d", 
    ZX_SMI_WIDTH, 
    ZX_SMI_NS, 
    ZX_SMI_SETUP, 
    ZX_SMI_STROBE, 
    ZX_SMI_HOLD, 
    ZX_SMI_PACE);

  m_SMIMaster.SetupTiming(
    ZX_SMI_WIDTH, 
    ZX_SMI_NS, 
    ZX_SMI_SETUP, 
    ZX_SMI_STROBE, 
    ZX_SMI_HOLD, 
    ZX_SMI_PACE,
    0,
    ZX_SMI_EXTERNAL_DREQ
  );

  // Setup SMI DMA and buffers (must be 64-bit word aligned, see /doc/dma-buffer-requirements.txt)  
  LOGDBG("Setting SMI DMA: Buffer length: %d bytes", m_nDMABufferLenBytes);

  // Configure GPIO interrupt
#if (ZX_SMI_USE_FIQ)  
  m_GpioFiqPin.ConnectInterrupt(GpioIrqHandler, this, FALSE);
#else
  m_GpioIrqPin.ConnectInterrupt(GpioIrqHandler, this, FALSE);
#endif  

  // Configure SMI DMA
  m_SMIMaster.SetupDMA(m_nDMABufferLenBytes, TRUE);

  // Configure DMA control blocks
  for (unsigned i = 0; i < ZX_DMA_BUFFER_COUNT; i++) {
    // Set up reads in separate control blocks
    m_pDMAControlBlocks[i]->SetupIORead (m_pDMABuffers[i], ARM_SMI_D, m_nDMABufferLenBytes, DREQSourceSMI);  
    m_pDMAControlBlocks[i]->SetWaitForWriteResponse(TRUE);
    m_pDMAControlBlocks[i]->SetBurstLength(8);

    // Ensure CBs are not cached (Start should do this! ..and it does do for this first one, but not chained ones)
    TDMAControlBlock *pCB = m_pDMAControlBlocks[i]->GetRawControlBlock();
    CleanAndInvalidateDataCacheRange ((uintptr) pCB, sizeof *pCB); // Ensure cb not cached
  }

  // PeripheralExit ();

	return TRUE;
}

void CZxSmi::Start(CSynchronizationEvent *pFrameEvent)
{
  LOGDBG("Starting SMI DMA");

  // Set running flag
  m_bRunning = TRUE;

  // Set frame event
  m_pFrameEvent = pFrameEvent;

  // Set initial DMA buffer and control block  
  m_pDMABuffer = m_pDMABuffers[m_nDMABufferIdx];
  m_pDMAControlBlock = m_pDMAControlBlocks[m_nDMABufferIdx];

  // Set DMA completion routines
  // m_DMA.SetCompletionRoutine(DMACompleteInterrupt, this);

  // Set SMI completion routines
  m_SMIMaster.SetCompletionRoutine(SMICompleteInterrupt, this);
  
  DMAStart();  

  // Only start interrupt after DMA has been configured
#if (ZX_SMI_USE_FIQ)    
  m_GpioFiqPin.EnableInterrupt(GPIOInterruptOnFallingEdge);
#else
  m_GpioIrqPin.EnableInterrupt(GPIOInterruptOnFallingEdge);
  // m_GpioIrqPin.EnableInterrupt(GPIOInterruptOnAsyncFallingEdge);
#endif  

  // m_DMA.Wait();

  // if (m_pActLED != nullptr) {
  //   m_pActLED->On();
  // }

  

  // DMAStart();  

  // m_DMA.Wait();

  // if (m_pActLED != nullptr) {
  //   m_pActLED->Off();
  // }
}

void CZxSmi::Stop()
{
  LOGDBG("Stopping SMI DMA");

  m_bRunning = FALSE;
  m_pFrameEvent = nullptr;

  // TODO - stop the hardware / cancel the DMA if possible
}

void CZxSmi::SetActLED(CActLED *pActLED) {
  m_pActLED = pActLED;
}

// void CZxSmi::SetBufferReadyRoutine (TSMICompletionRoutine *pRoutine, void *pParam) {

// }

volatile ZX_DMA_T CZxSmi::GetValue() {
  return m_value;
}

ZX_DMA_T *CZxSmi::LockDataBuffer(void) {
  // Lock the buffer
  m_DMABufferReadSemaphore.Down();

  // Ensure DMA buffer is not cached
  CleanAndInvalidateDataCacheRange((uintptr)m_pDMABufferRead, m_nDMABufferLenBytes);


  // Return the buffer
  return m_pDMABufferRead;
}

void CZxSmi::ReleaseDataBuffer(void) {
    // Zero out the DMA buffer, read for next fill
  memset(m_pDMABufferRead, 0, m_nDMABufferLenBytes);
  CleanAndInvalidateDataCacheRange((uintptr)m_pDMABufferRead, m_nDMABufferLenBytes); // Necessary?


  // Release the buffer
  m_DMABufferReadSemaphore.Up();
}

//
// Private
//

// TODO - rename to DMAConfigure
void CZxSmi::DMAStart()
{
//
}

// void CZxSmi::DMAStart()
// {
//   // CDMAChannel dma = m_DMABufferIdx == 0 ? m_DMA : m_DMA_2;

//   // Configure the DMARestart_1 channel to do something
  
//   // // Prepare some dummy data
//   // unsigned dataLen = 1024 * 1024 * 100;  // 100MB!
//   // char *pDataIn = new char[dataLen];
//   // char *pDataOut = new char[dataLen];
//   // strcpy(pDataIn, "HELLO WHIRLED!");
//   // CleanAndInvalidateDataCacheRange((uintptr)pDataIn, dataLen);

//   // m_DMA.SetupMemCopy(pDataOut, pDataIn, dataLen);

//   // m_DMA.Start();

//   // m_DMARestart_1.Prepare();

//   // m_SMIMaster.StartDMA(m_DMA, m_pDMABuffer);  



//   // CleanAndInvalidateDataCacheRange((uintptr)m_pOffscreenBuffer, m_nOffscreenBufferSize);
//   //   CleanAndInvalidateDataCacheRange((uintptr)m_pOffscreenBuffer2, m_nOffscreenBufferSize);

//   //   m_pDMAControlBlock->SetupMemCopy (pScreenBuffer, m_pOffscreenBuffer, m_nOffscreenBufferSize, ZX_SCREEN_DMA_BURST_COUNT, FALSE);
//   //   m_pDMAControlBlock->SetWaitCycles(32);
//   //   m_pDMAControlBlock2->SetupMemCopy (pScreenBuffer, m_pOffscreenBuffer2, m_nOffscreenBufferSize, ZX_SCREEN_DMA_BURST_COUNT, FALSE);
//   //   m_pDMAControlBlock2->SetWaitCycles(32);
    
//   //   // Chain
//   //   m_pDMAControlBlock->SetNextControlBlock (m_pDMAControlBlock2);
//   //   // m_pDMAControlBlock2->SetNextControlBlock (m_pDMAControlBlock);

//   //   // Ensure CBs are not cached (Start should do this! ..and it does do for this first one, but not chained ones)
//   //   TDMAControlBlock *pCB1 = m_pDMAControlBlock->GetRawControlBlock();
//   //   TDMAControlBlock *pCB2 = m_pDMAControlBlock2->GetRawControlBlock();
//   //   CleanAndInvalidateDataCacheRange ((uintptr) pCB1, sizeof *pCB1); // Ensure cb not cached
//   //   CleanAndInvalidateDataCacheRange ((uintptr) pCB2, sizeof *pCB2); // Ensure cb not cached

//   //   m_DMA.Start (m_pDMAControlBlock);

//   // // Prepare some dummy data
//   unsigned dataLen = 1024 * 1024 * 1;  // 1MB!
//   char *pDataIn = new char[dataLen];
//   char *pDataOut = new char[dataLen];
//   strcpy(pDataIn, "HELLO WHIRLED!");

//   // Calculate the ARM_SM_CS address for the GPU
//   u32 ARM_SMI_CS_GPU = ARM_SMI_CS;
// 	ARM_SMI_CS_GPU = ARM_SMI_CS_GPU &= 0xFFFFFF;
// 	ARM_SMI_CS_GPU += GPU_IO_BASE;

//   u32 *pARM_SMI_CS = (u32 *)ARM_SMI_CS_GPU;
//   // *((u32 *)pDataIn) = read32(ARM_SMI_CS) | CS_START;
//   *((u32 *)pDataIn) = read32(ARM_SMI_CS) | CS_START | CS_INTD;  // Enable the DONE interrupt

//   CleanAndInvalidateDataCacheRange((uintptr)pDataIn, dataLen);

//   unsigned dmaBufferLenWords = ZX_DMA_BUFFER_LENGTH;
//   unsigned dmaBufferLenBytes = ZX_DMA_BUFFER_LENGTH * sizeof(ZX_DMA_T);
  
//   // m_pDMAControlBlock1->SetupMemCopy (pDataOut, pDataIn, 4);
//   m_pDMAControlBlock1->SetupIOWrite (ARM_SMI_CS, pDataIn, 4, DREQSourceNone);
//   m_pDMAControlBlock2->SetupIORead (m_pDMABuffer, ARM_SMI_D, dmaBufferLenBytes, DREQSourceSMI);

//   // Chain
//   m_pDMAControlBlock1->SetNextControlBlock (m_pDMAControlBlock2);
//   m_pDMAControlBlock2->SetNextControlBlock (m_pDMAControlBlock1);

//   // Ensure CBs are not cached (Start should do this! ..and it does do for this first one, but not chained ones)
//   TDMAControlBlock *pCB1 = m_pDMAControlBlock1->GetRawControlBlock();
//   TDMAControlBlock *pCB2 = m_pDMAControlBlock2->GetRawControlBlock();
//   CleanAndInvalidateDataCacheRange ((uintptr) pCB1, sizeof *pCB1); // Ensure cb not cached
//   CleanAndInvalidateDataCacheRange ((uintptr) pCB2, sizeof *pCB2); // Ensure cb not cached


//   // PeripheralEntry();
// 	// write32(ARM_SMI_CS, read32(ARM_SMI_CS) | CS_START);
// 	// PeripheralExit();

//   CTimer::SimpleMsDelay(5);

  // m_DMA.Start (m_pDMAControlBlock1);

  
//   CTimer::SimpleMsDelay(500);
//   CleanAndInvalidateDataCacheRange((uintptr)pDataOut, dataLen);
//   LOGDBG("pARM_SMI_CS: 0x%08lx", pARM_SMI_CS);
//   LOGDBG("pDataIn: 0x%08lx", *((u32 *)pDataIn));
//   LOGDBG("ARM_SMI_CS: 0x%08lx", read32(ARM_SMI_CS));
//   // LOGDBG("pDataIn: 0x%08lx", *((u32 *)pDataIn+1));
//   // LOGDBG("pDataIn: 0x%08lx", *((u32 *)pDataIn+2));
//   // LOGDBG("pDataIn: 0x%08lx", *((u32 *)pDataIn+3));
//   // LOGDBG("pDataIn: 0x%08lx", *((u32 *)pDataIn+4));
//   // LOGDBG("pDataOut: 0x%08lx", *((u32 *)pDataOut));
//   // LOGDBG("pDataOut: 0x%08lx", *((u32 *)pDataOut+1));
//   // LOGDBG("pDataOut: 0x%08lx", *((u32 *)pDataOut+2));
//   // LOGDBG("pDataOut: 0x%08lx", *((u32 *)pDataOut+3));
//   // LOGDBG("pDataOut: 0x%08lx", *((u32 *)pDataOut+4));

//   if (m_pActLED != nullptr) {
//     m_pActLED->On();
//   }
// }

void CZxSmi::DMACompleteInterrupt(unsigned nChannel, boolean bStatus, void *pParam)
{
  // unsigned i = 0;

  CZxSmi *pThis = (CZxSmi *) pParam;
	assert (pThis != 0);

  // pThis->m_pActLED->On();

  // if (pThis->m_pActLED != nullptr) {
  //   pThis->m_pActLED->Off();
  // }

  // Return early if stopped
  if (!pThis->m_bRunning) return;

  // // CDMAChannel dma = pThis->m_DMABufferIdx == 0 ? pThis->m_DMA : pThis->m_DMA_2;

  // // Save pointer to buffer just filled by DMA
  // ZX_DMA_T *pBuffer = pThis->m_pDMABuffer;

  // // Move to the next DMA buffer
  // pThis->m_DMABufferIdx = (pThis->m_DMABufferIdx + 1) % ZX_DMA_BUFFER_COUNT;
  // pThis->m_pDMABuffer = pThis->m_pDMABuffers[pThis->m_DMABufferIdx];

  // (Re-)Start the DMA
  // pThis->DMAStart();
  // pThis->m_SMIMaster.HackDMA();
  // pThis->m_DMA.Start();

  // // Process the buffer (TODO - this should happen in a bottom half)
  // // boolean have1 = FALSE;
  // // for (unsigned i = 0; i < ZX_DMA_BUFFER_LENGTH; i++) {    
  // //   if (pBuffer[i] > 0) {
  // //     have1 = TRUE;
  // //   }
  // // }

  // // if (have1) {
  // //   pThis->m_bDMAResult = !pThis->m_bDMAResult;
  // // }

  // // if (pThis->m_bDMAResult) {
  // //   pThis->m_pActLED->On();
  // // } else {
  // //   pThis->m_pActLED->Off();
  // // }

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
  //       pThis->m_counter2 = 0;
  //     }      
  //     else if (pThis->m_isIOWrite && (value & ((1 << 15) | (1 << 14) | (1 << 13))) == 0) {
  //       // pThis->m_value = value & 0x07;  // Only interested in first 3 bits for the border colour
  //       pThis->m_counter2++;

  //       if (pThis->m_counter2 > 5) {
  //         pThis->m_value = value;
  //         pThis->m_isIOWrite = false;        
  //       }
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

    // CleanAndInvalidateDataCacheRange ((uintptr) pThis->m_DMA.m_pControlBlock, sizeof *(pThis->m_DMA.m_pControlBlock)); // Ensure cb not cached
    pThis->m_src =nChannel;// bStatus; //pThis->m_DMA.m_pControlBlock->nSourceAddress;
    // pThis->m_dst = pThis->m_DMA.m_pControlBlock->nDestinationAddress;

    // pThis->m_bDMAResult = !pThis->m_bDMAResult;
    // if (pThis->m_bDMAResult) {
    //   pThis->m_pActLED->On();
    // } else {
    //   pThis->m_pActLED->Off();
    // }

    
    if (pThis->m_counter <= 0) {
      pThis->m_counter = 10;
      pThis->m_bDMAResult = !pThis->m_bDMAResult;
    
      if (pThis->m_bDMAResult) {
        pThis->m_pActLED->On();
      } else {
        pThis->m_pActLED->Off();
      }
    }
    pThis->m_counter--;
}

void CZxSmi::DMARestartCompleteInterrupt(unsigned nChannel, boolean bStatus, void *pParam)
{
    CZxSmi *pThis = (CZxSmi *) pParam;
  	assert (pThis != 0);

    // pThis->m_pActLED->On();
}

void CZxSmi::SMICompleteInterrupt(boolean bStatus, void *pParam)
{
  // unsigned i = 0;

  CZxSmi *pThis = (CZxSmi *) pParam;
	assert (pThis != 0);


  // if (pThis->m_pActLED != nullptr) {
  //   pThis->m_pActLED->Off();
  // }

    if (pThis->m_counter <= 0) {
      pThis->m_counter = 10;
      pThis->m_bDMAResult = !pThis->m_bDMAResult;
    
      if (pThis->m_bDMAResult) {
        pThis->m_pActLED->On();
      } else {
        pThis->m_pActLED->Off();
      }
    }
    pThis->m_counter--;
}


// Try NO_USB_SOF_INTR to improve timing (use this option if it works!!)
// Try NO_BUSY_WAIT to deactivate busywaits - check note for multicore (we shouldn't need multicore)
// Might have to use CriticalSection and flags to pass data to tasks for FIQ
void CZxSmi::GpioIrqHandler (void *pParam) 
{
  CZxSmi *pThis = (CZxSmi *) pParam;
  assert (pThis != 0);

  // Return early if stopped
  if (!pThis->m_bRunning) return;


  // Toggle LED for debug
  pThis->m_pActLED->On();

  // This delay is important to ensure that the last video bytes are flushed out of the SMI FIFO when running
  // at slower sample rates. Without this, the last few bytes of the frame are lost.
  // At higher sample rates the delay is not needed, but it makes more sense to have a lower sample rate and
  // therefore lower power consumption.
  CTimer::SimpleusDelay(1);

  // Attempt to get the buffer lock - if it is still in use, we need to drop a frame
  bool haveLock = pThis->m_DMABufferReadSemaphore.TryDown();

  if (haveLock) {
    // We have the lock

    // // Stop the current DMA transfer if still in progress (NOT SURE IF NECESSARY)
    // pThis->m_DMA.Stop(); // Now in SM->StopDMA()

    // Stop the DMA and SMI
    pThis->m_SMIMaster.StopDMA(pThis->m_DMA);
    

    // Save pointer to buffer just filled by DMA
    pThis->m_pDMABufferRead = pThis->m_pDMABuffer;

    skippy++;

    if (skippy % 1 == 0) {

      // Move to the next DMA buffer and control block
      pThis->m_nDMABufferIdx = (pThis->m_nDMABufferIdx + 1) % ZX_DMA_BUFFER_COUNT;
      pThis->m_pDMABuffer = pThis->m_pDMABuffers[pThis->m_nDMABufferIdx];
      pThis->m_pDMAControlBlock = pThis->m_pDMAControlBlocks[pThis->m_nDMABufferIdx];

      // Start the DMA using the next control block
      pThis->m_DMA.Start (pThis->m_pDMAControlBlock);

      // // Start the SMI for DMA (DMA should already be ready to go)
      PeripheralEntry();
      write32(ARM_SMI_CS, read32(ARM_SMI_CS) | CS_START | CS_ENABLE); // No completion interrupt
      PeripheralExit();

    }

    // Release the lock
    pThis->m_DMABufferReadSemaphore.Up();

    // Signal the screen processor task
    if (pThis->m_pFrameEvent) {
      pThis->m_pFrameEvent->Set();
    }

  } else {
    // Drop a frame - the screen processor can't keep up
    skippedFrameCount++;
  }


  // Toggle LED for debug
  pThis->m_pActLED->Off();

  // Increment the loop count
  frameInterruptCount++;

  // Acknowledge the interrupt
  #if (ZX_SMI_USE_FIQ)    
    pThis->m_GpioFiqPin.AcknowledgeInterrupt();
  #else
    pThis->m_GpioIrqPin.AcknowledgeInterrupt();
  #endif  
}

// void CZxSmi::GpioIrqHandler (void *pParam) 
// {
//   CZxSmi *pThis = (CZxSmi *) pParam;
//   assert (pThis != 0);

// // #if (ZX_SMI_USE_FIQ)    
// //   // TODO - auto-ack?
// // #else
// //   pThis->m_GpioIrqPin.AcknowledgeInterrupt();
// // #endif  

  

//   // Return early if stopped
//   if (!pThis->m_bRunning) return;


//   // Toggle LED for debug
//   pThis->m_pActLED->On();


//   // CTimer::SimpleusDelay(2000);

//   // // Stop the current DMA transfer if still in progress (NOT SURE IF NECESSARY)
//   // pThis->m_DMA.Stop(); // Now in SM->StopDMA()

//   // // Stop the DMA and SMI
//   pThis->m_SMIMaster.StopDMA(pThis->m_DMA);
  

  

//   // Save pointer to buffer just filled by DMA
//   ZX_DMA_T *pBuffer = pThis->m_pDMABuffer;


//   // Move to the next DMA buffer and control block
//   pThis->m_nDMABufferIdx = (pThis->m_nDMABufferIdx + 1) % ZX_DMA_BUFFER_COUNT;
//   pThis->m_pDMABuffer = pThis->m_pDMABuffers[pThis->m_nDMABufferIdx];
//   pThis->m_pDMAControlBlock = pThis->m_pDMAControlBlocks[pThis->m_nDMABufferIdx];

//   // Start the DMA using the next control block
//   pThis->m_DMA.Start (pThis->m_pDMAControlBlock);

//   // // Start the SMI for DMA (DMA should already be ready to go)
//   PeripheralEntry();
//   write32(ARM_SMI_CS, read32(ARM_SMI_CS) | CS_START | CS_ENABLE); // No completion interrupt
//   PeripheralExit();

//   // Ensure previous DMA buffer is not cached
//   CleanAndInvalidateDataCacheRange((uintptr)pBuffer, pThis->m_nDMABufferLenBytes);

//   // Copy the DMA buffer to the screen buffer
//   // memcpy(pThis->m_pScreenDataBuffer, pBuffer, pThis->m_nDMABufferLenBytes);

//   // Clear the old DMA buffer
//   // memset(pBuffer, 0x00, pThis->m_nDMABufferLenBytes);
//   // CleanAndInvalidateDataCacheRange((uintptr)pBuffer, pThis->m_nDMABufferLenBytes);


//   // pThis->m_counter2 = read32(ARM_SMI_CS);

//   // GPIO27 (GPIO27, PIN13) => IRQ - IRQ for frame timing - !3S
//   // SD17   (GPI025, PIN22) => DREQ_ACK - UNUSED
//   // SD16   (GPI024, PIN18) => DREQ = /(/IORQ & /CAS) - triggers the SMI reads to DMA - !3S
//   // SD15   (GPI023, PIN16) => D0 - 3S
//   // SD14   (GPI022, PIN15) => D1 - 3S
//   // SD13   (GPI021, PIN40) => D2 - 3S
//   // SD12   (GPIO20, PIN38) => D3 - 3S
//   // SD11   (GPIO19, PIN35) => D4 - 3S
//   // SD10   (GPIO18, PIN12) => D5 - 3S
//   // SD9    (GPIO17, PIN11) => D6 - 3S
//   // SD8    (GPI016, PIN36) => D7 - 3S
//   // SD7    (RXD0,   PIN10) <= UART for debug
//   // SD6    (TXD0,   PIN8 ) => UART for debug 
//   // SD5    (PWM1,   PIN33) == UNUSED
//   // SD4    (PWM0,   PIN32) == UNUSED
//   // SD3    (SCLK,   PIN23) => 
//   // SD2    (MOSI,   PIN19) => /IORQ - When low, indicates an IO read or write - 3S
//   // SD1    (MISO,   PIN21) => CAS - Used to detect writes to video memory - 3S
//   // SD0    (CE0,    PIN24) => /WR - When low, indicates a write - 3S
//   // SOE/SE (GPIO6,  PIN31) <= SMI CLK - Clock for SMI, Switch off except when debugging

//   // OLD!!
//   // SD3    (SCLK,   PIN23) => CAS - Used to detect writes to video memory - 3S
//   // SD2    (MOSI,   PIN19) => A0 - Address bit 0 (/IOREQ = /IORQ | A0) ? is A0 inverted? - 3S
//   // SD1    (MISO,   PIN21) => /WR - When low, indicates a write - 3S
//   // SD0    (CE0,    PIN24) => /IORQ - When low, indicates an IO read or write - 3S
//   // SOE/SE (GPIO6,  PIN31) <= SMI CLK - Clock for SMI, Switch off except when debugging

//   // NOTES
//   // - The /OE for the buffers should be generated before the DREQ for the SMI, so that the data is
//   //   available when the SMI is triggered. ACTUALLY, the timing is good the other way around....
//   //   The data will be valid at the 3rd sample, which is the sample before a high on /CAS which
//   //   indicates the end of the read.
//   // - If there are 4 samples between the /CAS highs, then it is a video read. If there are more
//   //   then it is NOT a video read. There should never be less.
//   // - There are 256*192 / 8 = 6144 bytes of pixels and 32*24 = 768 bytes of attributes. 
//   //   giving a total of 6912 bytes (0x1B00) of video memory.
//   // - The Speccy reads the video memory in 2 byte chunks. 1 byte is the 8 pixesl, and the other
//   //   byte is the attribute bits for those pixel. This should result in 6144 * 2 = 12288 reads (0x3000)
//   // - ULA CAS pluse is [~204ns (gap ~60ns) 226ns] x2 (with 86ns gap)
//   // - Shortest Z80 read is ~296ns
//   // - Therefore we only have a window of ~70ns difference to differentiate the 2 :/

//   // Do the rest is the bottom half!


//   // // For debug
//   // memcpy(pThis->m_pDebugBuffer, pBuffer, ZX_SMI_DEBUG_BUFFER_LENGTH * sizeof(ZX_DMA_T));
//   pDEBUG_BUFFER = pBuffer;

//   // Reset state before processing data (this interrupt occurs at start of screen refresh)
//   u32 lastlastlastValue = 0;
//   u32 lastlastValue = 0;
//   u32 lastValue = 0;
//   u32 lastCasValue = 0;
//   u32 inIOWR = 0;
//   u32 inCAS = 0;
//   videoByteCount = 0;
//   u32 screenValue0 = 0;
//   u32 screenValue1 = 0;
//   u32 casReadIdx = 0;
//   volatile ZX_DMA_T *pScreenBuffer = pThis->m_pScreenDataBuffer;
//   boolean isPixels = true;

//   // Loop all the data in the buffer
//   for (unsigned i = 0; i < ZX_DMA_BUFFER_LENGTH; i++) {   
//       // Read the value from the buffer
//       ZX_DMA_T value = pBuffer[i];

//       // Clear out the buffer as we read it (might be quicker than a memset(), but may well be slower too!! TODO: CHECK)
//       // pBuffer[i] = 0xFFFF;

//       // if (value != 0xFFFF) {
//         // pThis->m_value = value;

//   // SD2    (MOSI,   PIN19) => /IORQ - When low, indicates an IO read or write - 3S
//   // SD1    (MISO,   PIN21) => CAS - Used to detect writes to video memory - 3S
//   // SD0    (CE0,    PIN24) => /WR - When low, indicates a write - 3S
//   // SOE/SE (GPIO6,  PIN31) <= SMI CLK - Clock for SMI, Switch off except when debugging


//         boolean IORQ = (value & (1 << 2)) == 0;
//         boolean WR = (value & (1 << 0)) == 0;
//         // boolean A0 = (value & (1 << 2)) != 0;
//         boolean CAS = (value & (1 << 1)) == 0;
//         // boolean IOREQ = IORQ & A0;                
//         boolean IOREQ = IORQ;                
//         u32 data = (value >> 8);// & 0x7;

        

//         // Test
//         // if (CAS) {
//         //   pThis->m_value = 0xDDDD;
//         // }

//         // Handle ULA video read (/CAS in close succession (~100ns in-between each cas)
//         if (i > 0) {
//           if (CAS) {
//             // Entered or in CAS
//             lastCasValue = data;
//             inCAS++;
//           } else {
//             // if (inCAS == 3) {
//             //   if (casReadIdx == 0) {
//             //     screenValue0 = lastValue;
//             //     casReadIdx = 1;
//             //   } else {
//             //     screenValue1 = lastValue;
//             //     videoByteCount +=2;
//             //   }
//             // } else {
//             //   casReadIdx = 0;
//             // }
//             if (inCAS >= 3 && inCAS <= 6) {
//               // // pThis->m_value = lastValue;   
//               // if (videoByteCount < 0x3000) {
//                 videoByteCount++;                
//                 // pThis->m_value = inCAS;   
//                 // pThis->m_value =  0xDDDD;                 

//                 if (isPixels) {
//                   // HW Fix, rotate the 8 bits
//                   lastCasValue = reverseBits(lastCasValue);

//                   borderValue = lastCasValue;                  

//                   *pScreenBuffer++ = lastCasValue;                
//                 }
//                 isPixels = !isPixels;                 
//               // }
//               // if (videoByteCount >= 0x3000) break;
//             } 
//             inCAS = 0;
//           }
//         }

//         // Handle IO write (border colour)
//         // if (IOREQ & WR) {          
//         //   // Entered or in IO write
//         //   inIOWR++;          
//         // } else {
//         //   if (inIOWR > 5) {
//         //     // Exited IO write, use the last value
//         //     // pThis->m_value = lastValue;
//         //     // pThis->m_value =  0xDDDD;
//         //     borderValue = lastValue;
//         //   }
//         //   inIOWR = 0;
//         // }

//         lastlastlastValue = lastlastValue;
//         lastlastValue = lastValue;
//         lastValue = data;        

//         // // Check for /IOREQ = 0 (/IOREQ = /IORQ | /A0)
//         // if (!pThis->m_isIOWrite && (value & ((1 << 15) | (1 << 13))) == 0) {
//         //   // pThis->m_value = value; // & 0x07;  // Only interested in first 3 bits
//         //   pThis->m_isIOWrite = true;        
//         //   pThis->m_counter2 = 0;
//         //   }      
//         //   else if (pThis->m_isIOWrite && (value & ((1 << 15) | (1 << 14) | (1 << 13))) == 0) {
//         //   // pThis->m_value = value & 0x07;  // Only interested in first 3 bits for the border colour
//         //   pThis->m_counter2++;

//         //   if (pThis->m_counter2 > 5) {
//         //   pThis->m_value = value;
//         //   pThis->m_isIOWrite = false;        
//         //   }
//         // }
//       // }      
//       // if (value > 0) {
//       //   pThis->m_counter++;
//       // }
//   }


//   // pThis->m_value = videoByteCount;

//   memset(pBuffer, 0, pThis->m_nDMABufferLenBytes);
//   CleanAndInvalidateDataCacheRange((uintptr)pBuffer, pThis->m_nDMABufferLenBytes);



//   // Toggle LED for debug
//   pThis->m_pActLED->Off();

//   // TODO - hand off the buffer to the main thread

//   // Signal the bottom half
//   if (pThis->m_pFrameEvent) {
//     pThis->m_pFrameEvent->Set();
//   }

//     // if (pThis->m_counter <= 0) {
//     //   pThis->m_counter = 10;
//     //   pThis->m_bDMAResult = !pThis->m_bDMAResult;
    
//     //   if (pThis->m_bDMAResult) {
//     //     pThis->m_pActLED->On();
//     //   } else {
//     //     pThis->m_pActLED->Off();
//     //   }
//     // }
//     // pThis->m_counter--;

//     _loops++;

//   #if (ZX_SMI_USE_FIQ)    
//   // TODO - auto-ack?
//   #else
//     pThis->m_GpioIrqPin.AcknowledgeInterrupt();
//   #endif  
// }
	