//
// zxsmi.cpp
//
#include <zxsmi/zxsmi.h>
#include <circle/util.h>

// HACK FOR SMI
#include <circle/memio.h>


LOGMODULE ("ZxSmi");

#define GPIO_FIQ_PIN    27

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


//
// Class
//

CZxSmi::CZxSmi (CInterruptSystem *pInterruptSystem)
: m_pInterruptSystem (pInterruptSystem),
  m_GpioFiqPin (GPIO_FIQ_PIN, GPIOModeInput, pInterruptSystem),
  m_SMIMaster (ZX_SMI_DATA_LINES_MASK, ZX_SMI_USE_ADDRESS_LINES, ZX_SMI_USE_SOE_SE, ZX_SMI_USE_SWE_SRW, pInterruptSystem),
  m_DMA (ZX_DMA_CHANNEL_TYPE, m_pInterruptSystem),
	m_pDMAControlBlock(0),
  m_pDMAControlBlock1(0),
  m_pDMAControlBlock2(0),
  m_pDMAControlBlock3(0),
  m_pDMAControlBlock4(0),
	m_pDMABuffer(0),
	m_DMABufferIdx(0),
  m_bRunning(FALSE),  
  m_pActLED(0),
  m_bDMAResult(0),
  m_counter(0),
  m_value(0),
  m_isIOWrite(0),
  m_counter2(0),
  m_src(0)
{

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

  m_GpioFiqPin.DisableInterrupt ();
}

boolean CZxSmi::Initialize ()
{
  unsigned dmaBufferLenWords = ZX_DMA_BUFFER_LENGTH;
  unsigned dmaBufferLenBytes = ZX_DMA_BUFFER_LENGTH * sizeof(ZX_DMA_T);
  
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
    m_pDMABuffers[i] = new ZX_DMA_T[dmaBufferLenWords]; // using new makes the buffer cache-aligned, so suitable for DMA
  }



  // Setup SMI timing
  LOGDBG("Setting SMI timing: width: %d, ns: %d, setup: %d, strobe: %d, hold: %d, pace: %d", 
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
    ZX_SMI_PACE
  );

  // Setup SMI DMA and buffers (must be 64-bit word aligned, see /doc/dma-buffer-requirements.txt)  
  LOGDBG("Setting SMI DMA: Buffer length: %d bytes", dmaBufferLenBytes);

  // Configure GPIO FIQ interrupt
  m_GpioFiqPin.ConnectInterrupt(GpioFiqHandler, this);

  // Configure SMI DMA
  m_SMIMaster.SetupDMA(dmaBufferLenBytes, TRUE);

  // Configure DMA control blocks
  for (unsigned i = 0; i < ZX_DMA_BUFFER_COUNT; i++) {
    // Set up reads in separate control blocks
    m_pDMAControlBlocks[i]->SetupIORead (m_pDMABuffers[0], ARM_SMI_D, dmaBufferLenBytes, DREQSourceSMI);  

    // Ensure CBs are not cached (Start should do this! ..and it does do for this first one, but not chained ones)
    TDMAControlBlock *pCB = m_pDMAControlBlocks[i]->GetRawControlBlock();
    CleanAndInvalidateDataCacheRange ((uintptr) pCB, sizeof *pCB); // Ensure cb not cached
  }

  // PeripheralExit ();

	return TRUE;
}

void CZxSmi::Start()
{
  LOGDBG("Starting SMI DMA");

  // Set running flag
  m_bRunning = TRUE;

  // Set initial DMA buffer and control block  
  m_pDMABuffer = m_pDMABuffers[m_DMABufferIdx];
  m_pDMAControlBlock = m_pDMAControlBlocks[m_DMABufferIdx];

  // Set DMA completion routines
  // m_DMA.SetCompletionRoutine(DMACompleteInterrupt, this);

  // Set SMI completion routines
  m_SMIMaster.SetCompletionRoutine(SMICompleteInterrupt, this);
  
  DMAStart();  

  // Only start interrupt after DMA has been configured
  m_GpioFiqPin.EnableInterrupt(GPIOInterruptOnFallingEdge);

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

  // TODO - stop the hardware / cancel the DMA if possible
}

void CZxSmi::SetActLED(CActLED *pActLED) {
  m_pActLED = pActLED;
}

ZX_DMA_T CZxSmi::GetValue() {
  return m_value;
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

void CZxSmi::GpioFiqHandler (void *pParam) 
{
  CZxSmi *pThis = (CZxSmi *) pParam;
  assert (pThis != 0);

  // Toggle LED for debug
  pThis->m_pActLED->On();


  // Save pointer to buffer just filled by DMA
  ZX_DMA_T *pBuffer = pThis->m_pDMABuffer;

  // Move to the next DMA buffer and control block
  pThis->m_DMABufferIdx = (pThis->m_DMABufferIdx + 1) % ZX_DMA_BUFFER_COUNT;
  pThis->m_pDMABuffer = pThis->m_pDMABuffers[pThis->m_DMABufferIdx];
  pThis->m_pDMAControlBlock = pThis->m_pDMAControlBlocks[pThis->m_DMABufferIdx];


  pThis->m_DMA.Start (pThis->m_pDMAControlBlock);

  // Start the SMI for DMA (DMA should already be ready to go)
  PeripheralEntry();
	write32(ARM_SMI_CS, read32(ARM_SMI_CS) | CS_START);
	PeripheralExit();


  // Loop all the data in the buffer
  for (unsigned i = 0; i < ZX_DMA_BUFFER_LENGTH; i++) {   
      ZX_DMA_T value = pBuffer[i];
      if (value > 0) {
        pThis->m_counter++;
      }
  }



  // Toggle LED for debug
  pThis->m_pActLED->Off();

  // TODO - hand off the buffer to the main thread

    // if (pThis->m_counter <= 0) {
    //   pThis->m_counter = 10;
    //   pThis->m_bDMAResult = !pThis->m_bDMAResult;
    
    //   if (pThis->m_bDMAResult) {
    //     pThis->m_pActLED->On();
    //   } else {
    //     pThis->m_pActLED->Off();
    //   }
    // }
    // pThis->m_counter--;
}
	