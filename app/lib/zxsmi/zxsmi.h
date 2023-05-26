//
// zxsmi.h
//
#ifndef _zxsmi_h
#define _zxsmi_h

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

#define ZX_SMI_DEBUG_BUFFER_LENGTH		256

// SMI Data and address lines
// #define ZX_SMI_DATA_LINES_MASK		0b001111111111111111
// NOTE: pins SD6 and SD7 are used for the serial UART used for debugging!
#define ZX_SMI_DATA_LINES_MASK		0b111111111100111111
#define ZX_SMI_USE_ADDRESS_LINES	FALSE
#define ZX_SMI_USE_SOE_SE					TRUE
// #define ZX_SMI_USE_SOE_SE					FALSE
#define ZX_SMI_USE_SWE_SRW				FALSE
#define ZX_SMI_WIDTH							SMI16Bits
#define ZX_SMI_EXTERNAL_DREQ			TRUE
#define ZX_SMI_USE_FIQ					FALSE

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
#define ZX_SMI_NS		4
#define ZX_SMI_SETUP	3
#define ZX_SMI_STROBE	4
#define ZX_SMI_HOLD		3
#endif

// SMI DMA
#define ZX_DMA_T					u16
// #define ZX_DMA_BUFFER_LENGTH		((1024 * 1024 * 4) / sizeof(ZX_DMA_T))
// #define ZX_DMA_BUFFER_LENGTH		((1024 * 1024 * 2) / sizeof(ZX_DMA_T))
// #define ZX_DMA_BUFFER_LENGTH		((1024 * 1024) / sizeof(ZX_DMA_T))
#define ZX_DMA_BUFFER_LENGTH		((1024 * 1024) / sizeof(ZX_DMA_T))
// #define ZX_DMA_BUFFER_LENGTH		((768 * 1024) / sizeof(ZX_DMA_T))
// #define ZX_DMA_BUFFER_LENGTH		((512 * 1024) / sizeof(ZX_DMA_T))
// #define ZX_DMA_BUFFER_LENGTH		((256 * 1024) / sizeof(ZX_DMA_T))
// #define ZX_DMA_BUFFER_LENGTH		((220 * 1024) / sizeof(ZX_DMA_T))
// #define ZX_DMA_BUFFER_LENGTH		((160 * 1024) / sizeof(ZX_DMA_T))
// #define ZX_DMA_BUFFER_LENGTH		((144 * 1024) / sizeof(ZX_DMA_T))
// #define ZX_DMA_BUFFER_LENGTH		((128 * 1024) / sizeof(ZX_DMA_T))
// #define ZX_DMA_BUFFER_LENGTH		((96 * 1024) / sizeof(ZX_DMA_T))
// #define ZX_DMA_BUFFER_LENGTH		((64 * 1024) / sizeof(ZX_DMA_T))
// #define ZX_DMA_BUFFER_LENGTH		((32 * 1024) / sizeof(ZX_DMA_T))
// #define ZX_DMA_BUFFER_LENGTH		((1 * 1024) / sizeof(ZX_DMA_T))
#define ZX_DMA_BUFFER_COUNT			2
#define ZX_DMA_CHANNEL_TYPE		DMA_CHANNEL_NORMAL
// #define ZX_DMA_CHANNEL_TYPE		DMA_CHANNEL_LITE

// ZX SMI PIN MASKS
#define ZX_PIN_IOREQ		(1 << 15)
#define ZX_PIN_WR				(1 << 14)


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

	ZX_DMA_T *LockDataBuffer(void);
	void ReleaseDataBuffer(void);

private:
	void DMAStart(void);
	static void DMACompleteInterrupt(unsigned nChannel, boolean bStatus, void *pParam);
	static void DMARestartCompleteInterrupt(unsigned nChannel, boolean bStatus, void *pParam);
	static void SMICompleteInterrupt(boolean bStatus, void *pParam);	
	static void GpioIrqHandler (void *pParam);
	
private:
// HACK
public:
	// members ...
	CGPIOManager *m_pGPIOManager;
	CInterruptSystem *m_pInterruptSystem;
#if (ZX_SMI_USE_FIQ)	
	CGPIOPinFIQ	m_GpioFiqPin;
#else		
	CGPIOPin	m_GpioIrqPin;
#endif	
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
	/*volatile*/ ZX_DMA_T *m_pDMABufferRead;
	volatile unsigned m_nDMABufferIdxRead;
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
