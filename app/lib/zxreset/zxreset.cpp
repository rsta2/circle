//
// zxreset.cpp
//

#include "zxreset.h"
// #include <circle/stdarg.h>


LOGMODULE ("ZxReset");


/* Definitions */

/* External global variables */
//

/* Exported global variables */
//

/* Local variables */
//

/* Local functions */
//

//
// Class
//

CZxReset::CZxReset (CGPIOManager *pGPIOManager, bool bInitialResetStateHeld)
: m_pGPIOManager(pGPIOManager),
  m_GpioOutputPin(ZX_RESET_GPIO_OUTPUT_PIN, GPIOModeInput, m_pGPIOManager),
  m_bResetHeld (bInitialResetStateHeld)
{
  //
}

CZxReset::~CZxReset (void)
{
  //
}

/**
 * Initialise ZX Reset
 */
boolean CZxReset::Initialize ()
{
  LOGNOTE("Initializing ZX RESET");

  // Set the GPIO pin to output mode (after setting its initial state, so as not to accidentally reset the speccy)
  m_GpioOutputPin.SetMode(GPIOModeOutput);


  if (this->m_bResetHeld) {
    // Hold the reset line low
    this->HoldReset();
  } else {
    // Clear the reset line
    this->ClearReset();
  }

	return TRUE;
}

void CZxReset::Reset()
{
  LOGDBG("RESETTING");
  
  // Set low
  HoldReset();

  // Wait 300us for cap discharge
  CTimer::Get()->usDelay(ZX_RESET_PULSE_WIDTH_US);

  ClearReset();
}

void CZxReset::HoldReset()
{
  LOGDBG("Holding RESET");
  
  // NOTE: It takes about 300ms for the ZX Spectrum reset to go low, because the 2N3904 transistor is not fully 
  // on with the 3.3V at the base from the GPIO pin.

  // Set the reset GPIO high, draining the reset cap
  m_GpioOutputPin.Write(HIGH);
  m_bResetHeld = true;
}

void CZxReset::ClearReset()
{
  LOGDBG("Clearing RESET");


  // Set the reset GPIO low, switching off the transistor and allowing the reset cap to charge
  m_GpioOutputPin.Write(LOW);
  m_bResetHeld = true;
}  


//
// Private
//

