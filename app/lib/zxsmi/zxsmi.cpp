//
// zxsmi.cpp
//
#include <zxsmi/zxsmi.h>


LOGMODULE ("ZxSmi");

//
// Class
//

CZxSmi::CZxSmi (CInterruptSystem *pInterruptSystem)
: m_pInterruptSystem (pInterruptSystem),
  m_SMIMaster (ZX_SMI_DATA_LINES_MASK, ZX_SMI_USE_ADDRESS_LINES, ZX_SMI_USE_SOE_SE, ZX_SMI_USE_SWE_SRW, pInterruptSystem),
  m_DMA (ZX_DMA_CHANNEL_TYPE, m_pInterruptSystem),
  // m_DMA_2 (ZX_DMA_CHANNEL_TYPE, m_pInterruptSystem),
	m_pDMABuffer(0),
	m_DMABufferIdx(0),
  m_bRunning(FALSE),  
  m_pActLED(0),
  m_bDMAResult(0),
  m_counter(0),
  m_value(0),
  m_isIOWrite(0),
  m_counter2(0)
{
  
}

CZxSmi::~CZxSmi (void)
{
}

boolean CZxSmi::Initialize ()
{
  unsigned i;
  unsigned dmaBufferLenWords = ZX_DMA_BUFFER_LENGTH;
  unsigned dmaBufferLenBytes = ZX_DMA_BUFFER_LENGTH * sizeof(ZX_DMA_T);
  
  LOGNOTE("Initializing SMI");

  // PeripheralEntry ();
  

  // Allocate DMA buffers
  for (i = 0; i < ZX_DMA_BUFFER_COUNT; i++) {
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

  // Configure SMI DMA
  m_SMIMaster.SetupDMA(dmaBufferLenBytes, TRUE);


  // PeripheralExit ();

	return TRUE;
}

void CZxSmi::Start()
{
  LOGDBG("Starting SMI DMA");

  // Set running flag
  m_bRunning = TRUE;

  // Set initial DMA buffer
  m_pDMABuffer = m_pDMABuffers[m_DMABufferIdx];

  // Set DMA completion routine
  m_DMA.SetCompletionRoutine(DMACompleteInterrupt, this);
  // m_DMA_2.SetCompletionRoutine(DMACompleteInterrupt, this);
  
  DMAStart();  

  // if (m_pActLED != nullptr) {
  //   m_pActLED->On();
  // }

  // m_DMA.Wait();
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

void CZxSmi::DMAStart()
{
  // CDMAChannel dma = m_DMABufferIdx == 0 ? m_DMA : m_DMA_2;

  m_SMIMaster.StartDMA(m_DMA, m_pDMABuffer);  

  // if (m_pActLED != nullptr) {
  //   m_pActLED->On();
  // }

}

void CZxSmi::DMACompleteInterrupt(unsigned nChannel, boolean bStatus, void *pParam)
{
  // unsigned i = 0;

  CZxSmi *pThis = (CZxSmi *) pParam;
	assert (pThis != 0);

  // if (pThis->m_pActLED != nullptr) {
  //   pThis->m_pActLED->Off();
  // }

  // Return early if stopped
  if (!pThis->m_bRunning) return;

  // CDMAChannel dma = pThis->m_DMABufferIdx == 0 ? pThis->m_DMA : pThis->m_DMA_2;

  // Save pointer to buffer just filled by DMA
  ZX_DMA_T *pBuffer = pThis->m_pDMABuffer;

  // Move to the next DMA buffer
  pThis->m_DMABufferIdx = (pThis->m_DMABufferIdx + 1) % ZX_DMA_BUFFER_COUNT;
  pThis->m_pDMABuffer = pThis->m_pDMABuffers[pThis->m_DMABufferIdx];

  // (Re-)Start the DMA
  pThis->DMAStart();


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

  for (unsigned i = 0; i < ZX_DMA_BUFFER_LENGTH; i++) {   
    ZX_DMA_T value = pBuffer[i];
    if (value > 0) {
      pThis->m_counter++;

      // pThis->m_value = value;


      // TODO: BORDER detection is sort of working, but I have reduced the SMI clock duration
      // Instead, there should be an IOWrite counter.

      // INT length is 282ns (One scan line is 64us)

      // CAS ULA detection works by measuring time between 2 CAS pulses. Should be between 
      // 200ns - 400ns, but NOT longer, otherwise it's a CPU read

      // Check for /IOREQ = 0 (/IOREQ = /IORQ | /A0)
      if (!pThis->m_isIOWrite && (value & ((1 << 15) | (1 << 13))) == 0) {
        // pThis->m_value = value; // & 0x07;  // Only interested in first 3 bits
        pThis->m_isIOWrite = true;        
        pThis->m_counter2 = 0;
      }      
      else if (pThis->m_isIOWrite && (value & ((1 << 15) | (1 << 14) | (1 << 13))) == 0) {
        // pThis->m_value = value & 0x07;  // Only interested in first 3 bits for the border colour
        pThis->m_counter2++;

        if (pThis->m_counter2 > 5) {
          pThis->m_value = value;
          pThis->m_isIOWrite = false;        
        }
      }    
    }
  }
  
  if (pThis->m_counter > 10000000) {
    pThis->m_counter = 0;
    pThis->m_bDMAResult = !pThis->m_bDMAResult;
   
    if (pThis->m_bDMAResult) {
      pThis->m_pActLED->On();
    } else {
      pThis->m_pActLED->Off();
    }
  }

}





