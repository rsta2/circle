//
// zxtape.cpp
//

#include "zxtape.h"
#include "tzx-api.h"
#include "chuckie-egg.h"
#include <circle/stdarg.h>


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
bool pauseOn;  						                  	// Control pause state



/* Local variables */
CZxTape *pZxTape = nullptr;                   // Pointer to ZX Tape singleton instance
unsigned char *pFileSeek;                     // Pointer to current file seek position


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
	m_bButtonPlayPause (false),  
	m_bButtonStop (false),
  m_nLastControlTicks(0)
{
  pZxTape = this;
  pFileSeek = nullptr;
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
  pauseOn = false;
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
    // TODO - switch GPIO pin to output when playing

    // TZXLoop only runs if a file is playing, and keeps the buffer full.
    TZXLoop();

    // Hacked in here for now
    wave();
  } else {
    LowWrite();    // Keep output LOW while no file is playing.
    // TODO - switch GPIO pin to input when not playing
  }
}  



/**
 * Handle control
 */
void CZxTape::loop_control() {
  const unsigned ticks = CTimer::Get()->GetTicks();
  const unsigned elapsedMs = (ticks - m_nLastControlTicks) * 1000 / HZ;

  // Only run every 100ms
  if (elapsedMs < 100) return;
  // LOGDBG("loop_control: %d", elapsedMs);
  
  m_nLastControlTicks = ticks;

  // Handle Play / pause button
  if (check_button_play_pause()) {
    if (!m_bRunning) {
      // Play pressed, and stopped, so start playing
      play_file();
      // delay(200);
    } else {
      // Play pressed, and playing, so pause / unpause
      if (pauseOn) {
        // Unpause playback
        pauseOn = false;
      } else {
        // Pause playback
        pauseOn = true;
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

void CZxTape::play_file() {
  pauseOn = false;
  currpct = 100;

  TZXPlay(); 
  m_bRunning = true; 

  if (PauseAtStart) {
    pauseOn = true;
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

// Log a message
void Log(const char *pMessage, ...) {
  va_list var;
	va_start (var, pMessage);

  CLogger::Get ()->WriteV ("TZX", LogDebug, pMessage, var);

	va_end (var);
}	

// Disable interrupts
void noInterrupts() {
  // LOGDBG("noInterrupts");
}


// Enable interrupts
void interrupts() {
  // LOGDBG("interrupts");
}


// Set the mode of a GPIO pin (i.e. set correct pin to output)
void pinMode(unsigned pin, unsigned mode) {
  LOGDBG("pinMode(%d, %d)", pin, mode);
}


// Set the GPIO output pin low
void LowWrite() {
  LOGDBG("LowWrite");
}


// Set the GPIO output pin high
void HighWrite() {
  LOGDBG("HighWrite");
}


// Delay for a number of milliseconds
void delay(unsigned long time) {
  LOGDBG("delay(%lu)", time);
}


// Stop the current file playback
void stopFile() {
  if (!pZxTape) return;

  LOGDBG("stopFile");

  pZxTape->stop_file();
}


// Called to display the playback percent (at start)
void lcdTime() {
  LOGDBG("lcdTime");
}


// Called to display the playback percent (during playback)
void Counter2() {
  LOGDBG("Counter2");
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
  LOGDBG("filetype_open");

  pFileSeek = ChuckieEgg;

  return true;
}

static void filetype_close() {
  LOGDBG("filetype_close");
  pFileSeek = nullptr;
}

static int filetype_read(void* buf, unsigned long count) {
  LOGDBG("filetype_read(%lu)", count);

  for (unsigned long i = 0; i < count; i++) {
    if (pFileSeek >= ChuckieEgg + filesize) {
      break;
    }
    LOGDBG("%02x ", pFileSeek[i]);
  }

  memcpy(buf, pFileSeek, count);
  pFileSeek += count;

  return count;
}

static bool filetype_seekSet(uint64_t pos) {
  LOGDBG("filetype_seekSet(%lu)", pos);

  pFileSeek = ChuckieEgg + pos;

  return true;
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
  LOGDBG("timer_initialize");
}

static void timer_stop() {
  LOGDBG("timer_stop");
}

static void timer_setPeriod(unsigned long period) {
  LOGDBG("timer_setPeriod(%lu)", period);
}
