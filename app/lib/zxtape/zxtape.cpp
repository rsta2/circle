//
// zxtape.cpp
//

#include "zxtape.h"
#include "tzx-api.h"
#include "chuckie-egg.h"


LOGMODULE ("ZxTape");


/* Definitions */
// NONE

/* External global variables */
extern "C" bool PauseAtStart;					        // Set to true to pause at start of file
extern "C" byte currpct;			                // Current percentage of file played (in file bytes, so not 100% accurate)

/* Exported global variables */
char fileName[ZX_TAPE_MAX_FILENAME_LEN + 1];  // Current filename     
uint16_t fileIndex;                    	      // Index of current file, relative to current directory (generally set to 0)
FILETYPE entry, dir;        		              // SD card current file (=entry) and current directory (=dir) objects
unsigned long filesize;             		      // filesize used for dimensioning files
TIMER Timer;								                  // Timer configure a timer to fire interrupts to control the output wave (call wave())



/* Local variables */
CZxTape *pZxTape = nullptr;                   // Pointer to ZX Tape singleton instance


/* Local functions */
// File API
static void initializeFileType(FILETYPE *pFileType);
static bool filetype_open(FILETYPE* dir, uint32_t index, oflag_t oflag);
static void filetype_close();
static int filetype_read(void* buf, unsigned long count);
static bool filetype_seekSet(uint64_t pos);	

// Timer API
static void initializeTimer(TIMER *pTimer);
static void timer_initialize();
static void timer_stop();
static void timer_setPeriod(unsigned long period);

//
// Class
//

CZxTape::CZxTape (CGPIOManager *pGPIOManager, CInterruptSystem *pInterruptSystem)
: m_pGPIOManager(pGPIOManager),
  m_pInterruptSystem (pInterruptSystem),
  m_GpioOutputPin(ZX_TAPE_GPIO_OUTPUT_PIN, GPIOModeInput, m_pGPIOManager),
  m_bRunning (false),
  m_bPauseOn (false),
	m_bButtonPlayPause (false),  
	m_bButtonStop (false)
{
  pZxTape = this;
}

CZxTape::~CZxTape (void)
{
  pZxTape = nullptr;
}

/**
 * Initialise ZX Tape
 */
boolean CZxTape::Initialize ()
{
  LOGNOTE("Initializing ZX TAPE");

  // Initialise the TZX API
  strncpy(fileName, "ChuckieEgg.tzx", ZX_TAPE_MAX_FILENAME_LEN); // TODO: This is a hack
  fileIndex = 0;
  initializeFileType(&dir);
  initializeFileType(&entry);
  filesize = sizeof(ChuckieEgg);
  initializeTimer(&Timer);
  currpct = 0;
  PauseAtStart = false;

  TZXSetup();

	return TRUE;
}

void CZxTape::PlayPause()
{
  LOGDBG("Starting TAPE");
  
  // Start the tape / pause the tape / unpause the tape
  m_bButtonPlayPause = true;
}

void CZxTape::Stop()
{
  LOGDBG("Stopping TAPE");

  // Stop the tape
  m_bButtonStop = true;
}  

/**
 * Run the tape loop 
 */
void CZxTape::Update()
{
  // Playback loop
  loop_playback();

  //  Control loop
  loop_control();
}  


//
// Private
//

/**
 * Handle playback
 */
void CZxTape::loop_playback()
{
  if(m_bRunning /* Tape running */) {
    // TZXLoop only runs if a file is playing, and keeps the buffer full.
    TZXLoop();
  } else {
    // TODO digitalWrite(outputPin, LOW);    // Keep output LOW while no file is playing.
  }
}  



/**
 * Handle control
 */
void CZxTape::loop_control() {
  if(true /* 50ms has passed */) {

    // Handle Play / pause button
    if (check_button_play_pause()) {
      if (m_bRunning) {
        // Play pressed, and stopped, so start playing
        play_file();
        // delay(200);
      } else {
        // Play pressed, and playing, so pause / unpause
        if (m_bPauseOn) {
          // Unpause playback
          m_bPauseOn = false;
        } else {
          // Pause playback
          m_bPauseOn = true;
        }
        
      }
    }
    
    // Handle stop button
    if (check_button_stop()) {
      if (m_bRunning) {
        // Playing and stop pressed, so stop playing
        stop_file();
      }
    }
  }
}

void CZxTape::play_file() {
  currpct = 100;

  TZXPlay(); 
  m_bRunning = true; 

  if (PauseAtStart) {
    m_bPauseOn = true;
    TZXPause();
  }
}

void CZxTape::stop_file() {
  TZXStop();
  if(m_bRunning){
    
  }
  m_bRunning = false;
}

bool CZxTape::check_button_play_pause() {
  if (m_bButtonPlayPause) {
    m_bButtonPlayPause = false;
    return true;
  }
  return false;
}

bool CZxTape::check_button_stop() {
  if (m_bButtonStop) {
    m_bButtonStop = false;
    return true;
  }
  return false;
}

//
// TZX API
//

// Disable interrupts
void noInterrupts() {
  // TODO
}


// Enable interrupts
void interrupts() {
  // TODO
}


// Set the mode of a GPIO pin (i.e. set correct pin to output)
void pinMode(unsigned pin, unsigned mode) {
  // TODO
}


// Set the GPIO output pin low
void LowWrite() {
  // TODO
}


// Set the GPIO output pin high
void HighWrite() {
  // TODO
}


// Delay for a number of milliseconds
void delay(unsigned long time) {
  // TODO
}


// Stop the current file playback
void stopFile() {
  // TODO
}


// Called to display the playback percent (at start)
void lcdTime() {
  // TODO
}


// Called to display the playback percent (during playback)
void Counter2() {
  // TODO
}


//
// File API
//

static void initializeFileType(FILETYPE *pFileType) {
  pFileType->open = filetype_open;
  pFileType->close = filetype_close;
  pFileType->read = filetype_read;
  pFileType->seekSet = filetype_seekSet;
}

static bool filetype_open(FILETYPE* dir, uint32_t index, oflag_t oflag) {
  return false;
}

static void filetype_close() {

}

static int filetype_read(void* buf, unsigned long count) {
  return 0;
}

static bool filetype_seekSet(uint64_t pos) {
  return false;
}

//
// Timer API
//

static void initializeTimer(TIMER *pTimer) {
  pTimer->initialize = timer_initialize;
  pTimer->stop = timer_stop;
  pTimer->setPeriod = timer_setPeriod;
}

static void timer_initialize() {

}

static void timer_stop() {

}

static void timer_setPeriod(unsigned long period) {

}
