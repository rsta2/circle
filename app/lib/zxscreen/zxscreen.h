//
// zxscreen.h
//
#ifndef _zxscreen_h
#define _zxscreen_h

#include "../../config.h"
#include <circle/actled.h>
#include <circle/types.h>
#include <circle/interrupt.h>
#include <circle/bcmframebuffer.h>
#include <circle/dmachannel.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <zxutil/zxutil.h>	// For rand_32();

//
// Configurable defines
//

/**
 * ZX_SCREEN_DEPTH
 * 
 * 8 - 8-bit colour
 * 16 - 16-bit colour
 * 32 - 32-bit colour
 */
#ifndef ZX_SCREEN_DEPTH
#define ZX_SCREEN_DEPTH 16
#endif

/**
 * ZX_SCREEN_COLOUR
 * 
 * TRUE - colour
 * FALSE - black and white (for testing)
 */
#ifndef ZX_SCREEN_COLOUR
#define ZX_SCREEN_COLOUR TRUE
#endif

/**
 * ZX_SCREEN_DMA
 * 
 * TRUE - DMA used for copying data to the framebuffer
 * FALSE - memcpy used for copying data to the framebuffer (faster as don't have to wait for DMA to complete)
 */
#ifndef ZX_SCREEN_DMA
#define ZX_SCREEN_DMA FALSE	// Disabled as waiting for DMA to complete is slower than just doing a memcpy().
#endif

/**
 * ZX_SCREEN_DMA_BURST_COUNT
 * 
 * number - burst count for DMA
 */
#ifndef ZX_SCREEN_DMA_BURST_COUNT
#define ZX_SCREEN_DMA_BURST_COUNT 0
#endif

/**
 * ZX_SCREEN_REVERSE_DATA_BITS
 * 
 * TRUE - data bits will be reversed (fixes a hardware bug)
 * FALSE - data bits will not be reversed
 */
#ifndef ZX_SCREEN_REVERSE_DATA_BITS
#define ZX_SCREEN_REVERSE_DATA_BITS FALSE
#endif



//
// Fixed defines
//

#define ZX_SCREEN_PIXEL_WIDTH	256
#define ZX_SCREEN_PIXEL_HEIGHT	192
#define ZX_SCREEN_BORDER_WIDTH	32
#define ZX_SCREEN_BORDER_HEIGHT	32
#define ZX_SCREEN_WIDTH			(ZX_SCREEN_BORDER_WIDTH + ZX_SCREEN_PIXEL_WIDTH + ZX_SCREEN_BORDER_WIDTH)
#define ZX_SCREEN_HEIGHT		(ZX_SCREEN_BORDER_HEIGHT + ZX_SCREEN_PIXEL_HEIGHT + ZX_SCREEN_BORDER_HEIGHT)

#define ZX_SCREEN_FLASH_FRAMES	16	// Frames between flash changes
#define ZX_SCREEN_COLOR_LUT_SIZE 	256 	// (8 * 8 * 2 * 2) ink * paper * flash * bright
#define ZX_SCREEN_WIDESCREEN    TRUE
#define ZX_SCREEN_WIDESCREEN_WIDTH_RATIO  (64 / 45) // 5:4 => 16:9 ((5*9) / (4*16))



// really ((green) & 0x3F) << 5, but to have a 0-31 range for all colors
#define COLOR16(red, green, blue)	  (((red) & 0x1F) << 11 \
					| ((green) & 0x1F) << 6 \
					| ((blue) & 0x1F))

// BGRA (was RGBA with older firmware)
#define COLOR32(red, green, blue, alpha)  (((blue) & 0xFF)       \
					| ((green) & 0xFF) << 8  \
					| ((red) & 0xFF)   << 16 \
					| ((alpha) & 0xFF) << 24)

#define BLACK_COLOR	0

#define NORMAL_COLOR	BRIGHT_WHITE_COLOR
#define HIGH_COLOR	BRIGHT_RED_COLOR
#define HALF_COLOR	BLUE_COLOR

#if ZX_SCREEN_DEPTH == 8
	typedef u8 TScreenColor;

	#define RED_COLOR16			COLOR16 (170 >> 3, 0, 0)
	#define GREEN_COLOR16			COLOR16 (0, 170 >> 3, 0)
	#define YELLOW_COLOR16			COLOR16 (170 >> 3, 85 >> 3, 0)
	#define BLUE_COLOR16			COLOR16 (0, 0, 170 >> 3)
	#define MAGENTA_COLOR16			COLOR16 (170 >> 3, 0, 170 >> 3)
	#define CYAN_COLOR16			COLOR16 (0, 170 >> 3, 170 >> 3)
	#define WHITE_COLOR16			COLOR16 (170 >> 3, 170 >> 3, 170 >> 3)

	#define BRIGHT_BLACK_COLOR16		COLOR16 (85 >> 3, 85 >> 3, 85 >> 3)
	#define BRIGHT_RED_COLOR16		COLOR16 (255 >> 3, 85 >> 3, 85 >> 3)
	#define BRIGHT_GREEN_COLOR16		COLOR16 (85 >> 3, 255 >> 3, 85 >> 3)
	#define BRIGHT_YELLOW_COLOR16		COLOR16 (255 >> 3, 255 >> 3, 85 >> 3)
	#define BRIGHT_BLUE_COLOR16		COLOR16 (85 >> 3, 85 >> 3, 255 >> 3)
	#define BRIGHT_MAGENTA_COLOR16		COLOR16 (255 >> 3, 85 >> 3, 255 >> 3)
	#define BRIGHT_CYAN_COLOR16		COLOR16 (85 >> 3, 255 >> 3, 255 >> 3)
	#define BRIGHT_WHITE_COLOR16		COLOR16 (255 >> 3, 255 >> 3, 255 >> 3)
	
	#define RED_COLOR			1
	#define GREEN_COLOR			2
	#define YELLOW_COLOR			3
	#define BLUE_COLOR			4
	#define MAGENTA_COLOR			5
	#define CYAN_COLOR			6
	#define WHITE_COLOR			7

	#define BRIGHT_BLACK_COLOR		8
	#define BRIGHT_RED_COLOR		9
	#define BRIGHT_GREEN_COLOR		10
	#define BRIGHT_YELLOW_COLOR		11
	#define BRIGHT_BLUE_COLOR		12
	#define BRIGHT_MAGENTA_COLOR		13
	#define BRIGHT_CYAN_COLOR		14
	#define BRIGHT_WHITE_COLOR		15

#elif ZX_SCREEN_DEPTH == 16
	typedef u16 TScreenColor;

	#define RED_COLOR				COLOR16 (0xCD >> 3, 0 >> 3, 0 >> 3)
	#define GREEN_COLOR				COLOR16 (0 >> 3, 0xCD >> 3, 0 >> 3)
	#define YELLOW_COLOR			COLOR16 (0xCD >> 3, 0xCD >> 3, 0 >> 3)
	#define BLUE_COLOR				COLOR16 (0 >> 3, 0 >> 3, 0xCD >> 3)
	#define MAGENTA_COLOR			COLOR16 (0xCD >> 3, 0, 0xCD >> 3)
	#define CYAN_COLOR				COLOR16 (0 >> 3, 0xCD >> 3, 0xCD >> 3)
	#define WHITE_COLOR				COLOR16 (0xCD >> 3, 0xCD >> 3, 0xCD >> 3)

	#define BRIGHT_BLACK_COLOR		COLOR16 (85 >> 3, 85 >> 3, 85 >> 3)
	#define BRIGHT_RED_COLOR		COLOR16 (255 >> 3, 0 >> 3, 0 >> 3)
	#define BRIGHT_GREEN_COLOR		COLOR16 (0 >> 3, 255 >> 3, 0 >> 3)
	#define BRIGHT_YELLOW_COLOR		COLOR16 (255 >> 3, 255 >> 3, 0 >> 3)
	#define BRIGHT_BLUE_COLOR		COLOR16 (0 >> 3, 0 >> 3, 255 >> 3)
	#define BRIGHT_MAGENTA_COLOR	COLOR16 (255 >> 3, 0 >> 3, 255 >> 3)
	#define BRIGHT_CYAN_COLOR		COLOR16 (0 >> 3, 255 >> 3, 255 >> 3)
	#define BRIGHT_WHITE_COLOR		COLOR16 (255 >> 3, 255 >> 3, 255 >> 3)
#elif ZX_SCREEN_DEPTH == 32
	typedef u32 TScreenColor;

	#define RED_COLOR			COLOR32 (170, 0, 0, 255)
	#define GREEN_COLOR			COLOR32 (0, 170, 0, 255)
	#define YELLOW_COLOR			COLOR32 (170, 85, 0, 255)
	#define BLUE_COLOR			COLOR32 (0, 0, 170, 255)
	#define MAGENTA_COLOR			COLOR32 (170, 0, 170, 255)
	#define CYAN_COLOR			COLOR32 (0, 170, 170, 255)
	#define WHITE_COLOR			COLOR32 (170, 170, 170, 255)

	#define BRIGHT_BLACK_COLOR		COLOR32 (85, 85, 85, 255)
	#define BRIGHT_RED_COLOR		COLOR32 (255, 85, 85, 255)
	#define BRIGHT_GREEN_COLOR		COLOR32 (85, 255, 85, 255)
	#define BRIGHT_YELLOW_COLOR		COLOR32 (255, 255, 85, 255)
	#define BRIGHT_BLUE_COLOR		COLOR32 (85, 85, 255, 255)
	#define BRIGHT_MAGENTA_COLOR		COLOR32 (255, 85, 255, 255)
	#define BRIGHT_CYAN_COLOR		COLOR32 (85, 255, 255, 255)
	#define BRIGHT_WHITE_COLOR		COLOR32 (255, 255, 255, 255)
#else
	#error DEPTH must be 8, 16 or 32
#endif



// Data types
typedef struct ZX_COLORS {
	TScreenColor ink;
	TScreenColor paper;
	u32 flash;
	u32 bright;
} ZX_COLORS;


class CZxScreen
{
public:
	CZxScreen (unsigned nWidth, unsigned nHeight, unsigned nDisplay, CInterruptSystem *pInterruptSystem);
	~CZxScreen (void);

	// methods ...
	boolean Initialize(void);
	
	void Start(void);
	void Stop(void);
	void SetActLED(CActLED *pActLED);
	
	/// \return Screen width in pixels
	unsigned GetWidth (void) const;
	/// \return Screen height in pixels
	unsigned GetHeight (void) const;

	void Clear (TScreenColor backgroundColor);
	void SetBorder (TScreenColor borderColor);
	void SetScreen (boolean bToggle);
	void SetScreenFromULABuffer(u32 frameNo, u16 *pULABuffer, size_t len);

	void UpdateScreen(void);
private:
	void InitZxColorLUT(void);
	ZX_COLORS ZxAttrToScreenColors(u32 attr);
	TScreenColor ZxColorToScreenColor(u32 color);
	TScreenColor ZxColorInkToScreenColor(u32 ink);
	TScreenColor ZxColorPaperToScreenColor(u32 paper);

	void ULADataToScreen(u32 frameNo, TScreenColor *pScreenBuffer, u16 *pULABuffer, size_t nULABufferLen);
	void DMAStart(void);
	static void DMACompleteInterrupt(unsigned nChannel, boolean bStatus, void *pParam);

private:
	// members ...
	CInterruptSystem *m_pInterruptSystem;
	CBcmFrameBuffer	*m_pFrameBuffer;

	unsigned	 m_nInitWidth;
	unsigned	 m_nInitHeight;
	unsigned	 m_nDisplay;
	unsigned	 m_nPitch;
		
	unsigned	 m_nWidth;
	unsigned	 m_nHeight;
	unsigned	 m_nZxScreenPixelWidth;
	unsigned	 m_nZxScreenPixelHeight;
	unsigned	 m_nZxScreenBorderWidth;
	unsigned	 m_nZxScreenBorderHeight;	
	unsigned	 m_nZxScreenWidth;
	unsigned	 m_nZxScreenHeight;
	TScreenColor  	*m_pZxScreenBuffer;
	TScreenColor  	*m_pScreenBuffer;
	TScreenColor  	*m_pOffscreenBuffer;
	// TScreenColor  	*m_pOffscreenBuffer2;
	unsigned	 m_nScreenBufferSize;
	unsigned	 m_nOffscreenBufferSize;
	unsigned	 m_nZxScreenBufferSize;
	unsigned	 m_nPixelCount;
	unsigned	 m_nZxScreenPixelCount;
	unsigned	 m_nScreenBufferNo;
	boolean		 m_bDirty;

	ZX_COLORS zxColors[ZX_SCREEN_COLOR_LUT_SIZE];

#ifdef ZX_SCREEN_DMA
	CDMAChannel	 m_DMA;
	// CDMAChannel::CDMAControlBlock *m_pDMAControlBlock;
	// CDMAChannel::CDMAControlBlock *m_pDMAControlBlock2;
	// CDMAChannel::CDMAControlBlock *m_pDMAControlBlock3;
	// CDMAChannel::CDMAControlBlock *m_pDMAControlBlock4;
#endif	
	CSpinLock	 m_SpinLock;

	volatile boolean m_bRunning;
	CActLED *m_pActLED;


	// // Temp
	// volatile int m_bDMAResult;
	// volatile unsigned m_counter;
	// volatile ZX_DMA_T m_value;
	// volatile boolean m_isIOWrite;
};

#endif // _zxscreen_h
