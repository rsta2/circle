//
// zxreset.h
//
#ifndef _zxreset_h
#define _zxreset_h

#include "../../config.h"
#include <circle/types.h>
#include <circle/interrupt.h>
#include <circle/usertimer.h>
#include <circle/gpiomanager.h>
#include <circle/gpiopin.h>
#include <circle/gpiopinfiq.h>
#include <circle/logger.h>

//
// Configurable defines
//

/**
 * ZX_RESET_GPIO_OUTPUT_PIN
 * 
 * Selects the GPIO output pin to use for resetting the ZX Spectrum
 * 
 */
#ifndef ZX_RESET_GPIO_OUTPUT_PIN
#define ZX_RESET_GPIO_OUTPUT_PIN	0	// GPIO 0 (HW PIN 27)
#endif


//
// Fixed defines
//

// Length of the reset pulse in microseconds
#define ZX_RESET_PULSE_WIDTH_US  			300      		


class CZxReset
{
public:
	CZxReset (CGPIOManager *pGPIOManager, bool bInitialResetStateHeld);
	~CZxReset (void);

	// methods ...
	boolean Initialize(void);
	void Reset(void);
	void HoldReset(void);
	void ClearReset(void);
	bool IsResetHeld(void);

private:
	//

private:
	// members ...
	CGPIOManager *m_pGPIOManager;
	CGPIOPin	m_GpioOutputPin;

	bool m_bResetHeld;
};

#endif // _zxreset_h
