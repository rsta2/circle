/* HD_SPECCYS: START */

// Source: https://github.com/sadken/TZXDuino/blob/master/TZXDuino.h

#ifndef _txz_api_h
#define _txz_api_h


#include "tzx-types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define outputPin           9               // Audio Output PIN - Set accordingly to your hardware.
#define maxFilenameLength   1024       		// Maximum length for long filename support (ideally as large as possible to support very long filenames)


/* External Variables (to be implemented by consumer) */
extern char fileName[];							// Current filename     
extern uint16_t fileIndex;                    	// Index of current file, relative to current directory (set to 0)
extern FILETYPE entry, dir;        				// SD card current file (=entry) and current directory (=dir) objects
extern unsigned long filesize;             		// filesize used for dimensioning AY files
extern TIMER Timer;								// Timer configure a timer to fire interrupts to control the output wave (call wave())

/* External Variables (implemented in library) */
extern byte currpct;							// Current percentage of file played (in file bytes, so not 100% accurate)
extern bool PauseAtStart;						// Set to true to pause at start of file

/* External API function prototypes (to be implemented by consumer) */
extern void noInterrupts();	// Disable interrupts
extern void interrupts();	// Enable interrupts
extern void pinMode(unsigned pin, unsigned mode);	// Set the mode of a GPIO pin (i.e. set correct pin to output)
extern void LowWrite();	// Set the GPIO output pin low
extern void HighWrite();	// Set the GPIO output pin high
extern void delay(unsigned long time);	// Delay for a number of milliseconds
extern void stopFile();	// Stop the current file playback
extern void lcdTime(); 	// Called to display the playback percent (at start)
extern void Counter2(); // Called to display the playback percent (during playback)

/* API function prototypes */
void TZXSetup();
void TZXLoop();
void TZXPlay();
void TZXPause();
void TZXStop();
void wave();	// Function to call on Timer interrupt


#ifdef __cplusplus
}
#endif



#endif // _tzx_api_h

