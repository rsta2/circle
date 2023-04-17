//
// myclass.h
//
#ifndef _template_myclass_h
#define _template_myclass_h

#include <circle/actled.h>
#include <circle/types.h>
#include <circle/interrupt.h>
#include <circle/smimaster.h>
#include <circle/logger.h>

// SMI Data and address lines
#define ZX_SMI_DATA_LINES_MASK		0b001111111111111111
#define ZX_SMI_USE_ADDRESS_LINES	FALSE
#define ZX_SMI_USE_SOE_SE					TRUE
#define ZX_SMI_USE_SWE_SRW				TRUE
#define ZX_SMI_WIDTH							SMI16Bits

// SMI Timing (for a 50 ns cycle time, 20MHz)
// Timings for RPi v4 (1.5 GHz) - (10 * (15+30+15) / 1.5GHZ) = 400ns
// Timings for RPi v0-3 (1 GHz) - (10 * (10+20+10) / 1GHZ)   = 400ns
// Timings for RPi v4 (1.5 GHz) - (4 * (5+9+5) / 1.5GHZ)     = 50.7ns
// Timings for RPi v0-3 (1 GHz) - (10 * (10+20+10) / 1GHZ)   = 400ns
#define ZX_SMI_PACE		0
#if RASPI > 3
#define ZX_SMI_NS			10
#define ZX_SMI_SETUP	15
#define ZX_SMI_STROBE	30
#define ZX_SMI_HOLD		15
#else
#define ZX_SMI_NS			4
#define ZX_SMI_SETUP	5
#define ZX_SMI_STROBE	9
#define ZX_SMI_HOLD		5
#endif

// SMI DMA
#define ZX_DMA_T								u16
// #define ZX_DMA_BUFFER_LENGTH		((1024 * 1024) / sizeof(ZX_DMA_T))
#define ZX_DMA_BUFFER_LENGTH		((32 * 1024) / sizeof(ZX_DMA_T))
#define ZX_DMA_BUFFER_COUNT			2
#define ZX_DMA_CHANNEL_TYPE			DMA_CHANNEL_NORMAL /*DMA_CHANNEL_LITE*/

class CZxSmi
{
public:
	CZxSmi (CInterruptSystem *pInterruptSystem);
	~CZxSmi (void);

	// methods ...
	boolean Initialize(void);
	
	void Start(void);
	void Stop(void);

	void SetActLED(CActLED *pActLED);

private:
	void DMAStart(void);
	static void DMACompleteInterrupt(unsigned nChannel, boolean bStatus, void *pParam);

private:
	// members ...
	CInterruptSystem *m_pInterruptSystem;
	CSMIMaster m_SMIMaster;
	CDMAChannel m_DMA;
	ZX_DMA_T *m_pDMABuffers[ZX_DMA_BUFFER_COUNT];
	ZX_DMA_T *m_pDMABuffer;
	unsigned m_DMABufferIdx;
	boolean m_bRunning;
	CActLED *m_pActLED;

	// Temp
	int m_bDMAResult;
};

#endif
