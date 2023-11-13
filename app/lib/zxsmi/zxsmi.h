//
// zxsmi.h
//
#ifndef _zxsmi_h
#define _zxsmi_h

#include "../../config.h"
#include <circle/actled.h>
#include <circle/types.h>
#include <circle/interrupt.h>
#include <circle/gpiomanager.h>
#include <circle/gpiopin.h>
#include <circle/gpiopinfiq.h>
#include <circle/logger.h>
#include <circle/smimaster.h>
#include <circle/sched/synchronizationevent.h>
#include <circle/sched/semaphore.h>

//
// Configurable defines
//

/**
 * ZX_SMI_GPIO_INT_INPUT_PIN
 * 
 * Selects the GPIO output pin to use for the tape playback
 */
#ifndef ZX_SMI_GPIO_INT_INPUT_PIN
#define ZX_SMI_GPIO_INT_INPUT_PIN 	27 // GPIO 27 (HW PIN 13) - Rev 1.0a
#endif

/**
 * ZX_SMI_GPIO_BORDER_INPUT_PIN
 * 
 * Selects the GPIO output pin to use for the tape playback
 */
#ifndef ZX_SMI_GPIO_BORDER_INPUT_PIN
#define ZX_SMI_GPIO_BORDER_INPUT_PIN 	26 // GPIO 26 (HW PIN 37) - Rev 1.0a
#endif

/**
 * ZX_SMI_GPIO_USE_FIQ
 * 
 * Selects whether to use a FIQ or a standard IRQ for the SMI GPIO interrupt
 * 
 * TRUE - Use FIQ 
 * FALSE - Use a standard IRQ
 */
#ifndef ZX_SMI_GPIO_USE_FIQ
#define ZX_SMI_GPIO_USE_FIQ 	TRUE
#endif




//
// Fixed defines
//



// NOTE: If FIQ can be made to work, then the problem with the screen flicker 
// could be sovled, as all IRQs have same proio (or defined by SW, no preemption)
// except FIQ which pre-empts all other IRQs!!!

#define ZX_SMI_DEBUG_BUFFER_LENGTH		256

// SMI Data and address lines
// #define ZX_SMI_DATA_LINES_MASK		0b001111111111111111
// NOTE: pins SD6 and SD7 are used for the serial UART used for debugging!
#define ZX_SMI_DATA_LINES_MASK		0b111111111100111111
#define ZX_SMI_USE_ADDRESS_LINES	FALSE
#define ZX_SMI_USE_SOE_SE					TRUE	// Debugging
// #define ZX_SMI_USE_SOE_SE					FALSE
#define ZX_SMI_USE_SWE_SRW				FALSE
#define ZX_SMI_WIDTH					SMI16Bits
#define ZX_SMI_PACK_DATA				TRUE
// #define ZX_SMI_WIDTH					SMI16Bits
// #define ZX_SMI_PACK_DATA				FALSE
#define ZX_SMI_EXTERNAL_DREQ			TRUE


// SMI Timing (for a 50 ns cycle time, 20MHz)
// Timings for RPi v4 (1.5 GHz) - (10 * (15+30+15) / 1.5GHZ) = 400ns
// Timings for RPi v0-3 (1 GHz) - (10 * (10+20+10) / 1GHZ)   = 400ns
// Timings for RPi v4 (1.5 GHz) - (4 * (5+9+5) / 1.5GHZ)     = 50.7ns
// Timings for RPi v0-3 (1 GHz) - (10 * (10+20+10) / 1GHZ)   = 400ns
#define ZX_SMI_PACE		0
#if RASPPI > 3
// 4 * (6+7+6) / 1.5GHz = 50.7ns
#define ZX_SMI_NS		4
#define ZX_SMI_SETUP	6
#define ZX_SMI_STROBE	7
#define ZX_SMI_HOLD		6
#else
// 2 * (8+9+8) / 1GHz = 50.0ns
// #define ZX_SMI_NS		4
// #define ZX_SMI_SETUP	3
// #define ZX_SMI_STROBE	5
// #define ZX_SMI_HOLD		3
#define ZX_SMI_NS		4
#define ZX_SMI_SETUP	3
#define ZX_SMI_STROBE	5
#define ZX_SMI_HOLD		3


// Pretty good timing for real ULA
// #define ZX_SMI_NS		4
// #define ZX_SMI_SETUP	3
// #define ZX_SMI_STROBE	2
// #define ZX_SMI_HOLD		3
// inCAS (5-7)

// Also ok
// #define ZX_SMI_NS		4
// #define ZX_SMI_SETUP	3
// #define ZX_SMI_STROBE	5
// #define ZX_SMI_HOLD		3
// inCAS (3-5)
#endif

// SMI DMA
#define ZX_DMA_T					u16
// #define ZX_DMA_BUFFER_LENGTH		(1024 * 1024 * 4)
#define ZX_DMA_BUFFER_LENGTH		(1024 * 1024 * 2)
// #define ZX_DMA_BUFFER_LENGTH		(1024 * 1024)
// #define ZX_DMA_BUFFER_LENGTH		(1024 * 1024)   // This currently uses up our entire loop time (but not long enough for JetPac)!
// #define ZX_DMA_BUFFER_LENGTH		(820 * 1024)	// Solid in DD2, should try really complex game
// #define ZX_DMA_BUFFER_LENGTH		(768 * 1024)	// This is long enough for the test cart - not sure in real situations (not for DD2!)
// #define ZX_DMA_BUFFER_LENGTH		(1 * 1024)
#define ZX_DMA_BUFFER_COUNT			2
#define ZX_DMA_CHANNEL_TYPE		DMA_CHANNEL_NORMAL
// #define ZX_DMA_CHANNEL_TYPE		DMA_CHANNEL_LITE

// // ZX SMI PIN MASKS (UNUSED??)
// #define ZX_PIN_IOREQ		(1 << 15)
// #define ZX_PIN_WR				(1 << 14)

// Theorectically the border colour can be changed every IO write. 
// - On the Z80, it takes 4 clock cycles to perform an IO write. 
// - The ZX Spectrum runs at 3.5MHz, so 4 clock cycles is 1.14us.
// - A screen frame takes 50Hz, so 1/50 = 20ms.
// - So, the border colour can be changed 20ms / 1.14us = 17543 times per frame.
// - Each IO write we need to record a us timestamp, which is sizeof(unsigned) = 4 bytes.
//  
// Memory on the PI as a microcontroller is cheap! 
// We'll make the buffer 1024 * 20 = 20480 words long, which is 81920 bytes (80kB).
#define ZX_BORDER_TIME_T					u32	
#define ZX_BORDER_TIMING_BUFFER_LENGTH		(1024 * 20)

typedef struct {
	unsigned nFrame;
	unsigned nFramesRendered;
	unsigned nFramesDropped;
	unsigned nFrameStartTime;
	ZX_DMA_T *pDMABuffer;
	unsigned nBorderTimingCount;
	ZX_BORDER_TIME_T *pBorderTimingBuffer;
} ZX_VIDEO_RAW_DATA_T;

// typedef void ZXSMIBufferReadyRoutine (boolean bStatus, void *pParam);


class CZxSmi
{
public:
	CZxSmi (CGPIOManager *pGPIOManager, CInterruptSystem *pInterruptSystem);
	~CZxSmi (void);

	// methods ...
	boolean Initialize(void);
	
	void Start(CSynchronizationEvent *pFrameEvent);
	void Stop(void);
	void SetActLED(CActLED *pActLED);
	volatile ZX_DMA_T GetValue(void);

	ZX_VIDEO_RAW_DATA_T *LockDataBuffer(void);
	void ReleaseDataBuffer(void);

private:
	void DMAStart(void);
	static void DMACompleteInterrupt(unsigned nChannel, boolean bStatus, void *pParam);
	static void DMARestartCompleteInterrupt(unsigned nChannel, boolean bStatus, void *pParam);
	static void SMICompleteInterrupt(boolean bStatus, void *pParam);		
	static void GpioIntIrqHandler (void *pParam);	
	static void GpioBorderIrqHandler (void *pParam);
	

private:
// HACK
public:
	// members ...
	CGPIOManager *m_pGPIOManager;
	CInterruptSystem *m_pInterruptSystem;	
	CGPIOPin	m_GpioIntIrqPin;	
	CGPIOPin	m_GpioBorderIrqPin;

	CSMIMaster m_SMIMaster;

	CDMAChannel m_DMA;
	CDMAChannel::CDMAControlBlock *m_pDMAControlBlocks[ZX_DMA_BUFFER_COUNT];
	CDMAChannel::CDMAControlBlock *m_pDMAControlBlock;

	CDMAChannel::CDMAControlBlock *m_pDMAControlBlock1;
	CDMAChannel::CDMAControlBlock *m_pDMAControlBlock2;
	CDMAChannel::CDMAControlBlock *m_pDMAControlBlock3;
	CDMAChannel::CDMAControlBlock *m_pDMAControlBlock4;

	ZX_DMA_T *m_pDMABuffers[ZX_DMA_BUFFER_COUNT];
	/*volatile*/ ZX_DMA_T *m_pDMABuffer;
	volatile unsigned m_nDMABufferIdx;
	u32 m_nDMABufferLenBytes;
	u32 m_nDMABufferLenWords;
	/*volatile*/ ZX_VIDEO_RAW_DATA_T *m_pRawVideoData;

	ZX_BORDER_TIME_T *m_pBorderTimingBuffers[ZX_DMA_BUFFER_COUNT];
	ZX_BORDER_TIME_T *m_pBorderTimingBuffer;
	volatile unsigned m_nBorderTimingWriteIdx;

	CSemaphore m_DMABufferReadSemaphore;
	CSynchronizationEvent	*m_pFrameEvent;


	volatile boolean m_bRunning;
	CActLED *m_pActLED;

	// Temp
	volatile int m_bDMAResult;
	volatile unsigned m_counter;
	volatile ZX_DMA_T m_value;
	public: volatile u32 m_src;
	public: volatile u32 m_dst;
	volatile boolean m_isIOWrite;
	volatile unsigned m_counter2;
	ZX_DMA_T *m_pDebugBuffer;
};

#endif // _zxsmi_h
