//
// zxtape.cpp
//

#include "zxtape.h"
#include "tzx-api.h"
// #include "games/chuckie-egg.h" // TZX, OK
// #include "games/jsw.h" // TZX, OK
// #include "games/jsw2.h" // TZX, OK
// #include "games/gyro.h" // TAP, OK
// #include "games/spy-hunter.h" // TZX, OK
// #include "games/exolon.h" // TZX, OK
// #include "games/dynamitedan2Tap.h" // TAP, OK (not original loader)
// #include "games/greenberet.h" // TAP, OK
// #include "games/spellbound.h" // TZX, OK
#include "games/dynamitedan2.h" // TZX, OK!
// #include "games/jetpac.h" // TAP, OK!
// #include "games/brianbloodaxe.h" // TZX, OK!
// #include "games/arkanoidSpeedlock4.h" // TZX, OK!
// #include "games/arkanoid2Speedlock7.h" // TZX, OK!
// #include "games/bubblebobbleBleepload.h" // TZX, OK!
// #include "games/rainbowislands.h" // TZX, OK! (has multi-loader (STOP the tape not yet implemented))
// #include "games/720Degrees.h" // TZX, OK!
// #include "games/starquake.h" // TZX, OK!
// #include "games/dynamitedan.h" // TZX, OK!
// #include "games/back2school.h" // TZX, OK! Fast baud rate, fails if buffer less than 128+64+32 (e.g. 128+64)
// #include "games/cauldron.h" // TZX, OK!
#include <circle/stdarg.h>

// Tape loading failures might be to do with the HW - the capacitor is taking too long to charge / discharge

// TODO: Add reset function / connector to the HD-Speccys final design


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
unsigned char *pFile;                         // Pointer to current file
uint64_t nFileSeekIdx;                        // Current file seek position
unsigned tzxLoopCount = 0;                    // HACK to call wave less than loop count at start

// HACK
unsigned char* GAME = DD2;
unsigned long GAME_SIZE = sizeof(DD2);


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
static void timer_setPeriod(unsigned long periodUs);

//
// Class
//

CZxTape::CZxTape (CGPIOManager *pGPIOManager, CInterruptSystem *pInterruptSystem)
: m_pGPIOManager(pGPIOManager),
  m_pInterruptSystem (pInterruptSystem),
  m_GpioOutputPin(ZX_TAPE_GPIO_OUTPUT_PIN, GPIOModeInput, m_pGPIOManager),
  m_OutputTimer(pInterruptSystem, CZxTape::wave_isr, this, ZX_TAPE_USE_FIQ),
  m_bRunning (false),
	m_bButtonPlayPause (false),  
	m_bButtonStop (false),
  m_bEndPlayback (false),
  m_nEndPlaybackDelay (0),
  m_nLastControlTicks(0)
{
  pZxTape = this;
  nFileSeekIdx = 0;
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
  // strncpy(fileName, "ChuckieEgg.tap", ZX_TAPE_MAX_FILENAME_LEN); // TODO: This is a hack
  fileIndex = 0;
  initializeFileType(&dir);
  initializeFileType(&entry);
  filesize = 0;
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
 * The tape is started (it may still be paused)
 * 
 */
bool CZxTape::IsStarted(void) {
  return m_bRunning;
}

/**
 * The tape is playing (started and NOT paused)
 * 
 */
bool CZxTape::IsPlaying(void) {
  return m_bRunning && !pauseOn;
}

/**
 * The tape is paused (started AND paused)
 * 
 */
bool CZxTape::IsPaused(void) {
  return m_bRunning && pauseOn;
}

/**
 * The tape is stopped (!IsPlaying())
 * 
 */
bool CZxTape::IsStopped(void) {
  return !m_bRunning;
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
  if(m_bRunning && !m_bEndPlayback) {
    // If tape is running, and we are not ending playback, then run the TZX loop

    // TZXLoop only runs if a file is playing, and keeps the buffer full.
    TZXLoop();

    // Hacked in here for testing
    // wave();
  }
}  



/**
 * Handle control
 */
void CZxTape::loop_control() {
  const unsigned ticks = CTimer::Get()->GetTicks();
  const unsigned elapsedMs = (ticks - m_nLastControlTicks) * 1000 / HZ;

  // Only run every 100ms
  if (elapsedMs < ZX_TAPE_CONTROL_UPDATE_MS) return;
  //  LOGDBG("loop_control: %d, m_bEndPlayback: %d, m_nEndPlaybackDelay: %d", elapsedMs, m_bEndPlayback, m_nEndPlaybackDelay);
  
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

  // Handle end of playback (allowing buffer to empty)
  if (m_bEndPlayback) {
    if (m_nEndPlaybackDelay <= 0) {
      // End of playback delay has expired, buffer should be empty, so stop playing
      stop_file();
    }
    m_nEndPlaybackDelay -= elapsedMs;
  }  
}

void CZxTape::play_file() {
  // Ensure stopped
  stop_file();

  // Set initial playback state
  pauseOn = false;
  currpct = 100;

  // Set GPIO pin to output mode (ensuring it is LOW)
  m_GpioOutputPin.Write(LOW);
  m_GpioOutputPin.SetMode(GPIOModeOutput);

  // Initialise the output timer
  m_OutputTimer.Initialize();

  TZXPlay(); 
  m_bRunning = true; 

  if (PauseAtStart) {
    pauseOn = true;
    TZXPause();
  }
}

void CZxTape::stop_file() {
  LOGDBG("stop_file");

  // Set GPIO pin to input mode
  m_GpioOutputPin.SetMode(GPIOModeInput);


  if(m_bRunning){
    LOGDBG("m_OutputTimer.Stop()");

    // Stop the output timer (only if it was initialised)
    m_OutputTimer.Stop();
  }

  // Stop tzx library
  TZXStop();
  
  m_bRunning = false;
  m_bEndPlayback = false;
  m_nEndPlaybackDelay = 0;
}

void CZxTape::end_playback() {
  m_bEndPlayback = true;
  m_nEndPlaybackDelay = ZX_TAPE_END_PLAYBACK_DELAY_MS;

  // Ensure last bits are written
  // This is not handled correctly in the TZXDuino code. 'count=255;' is decremented, and the buffer filled with junk,
  // but this happens much faster than the buffer can empty itself, so the last bytes/bits are not played back.
  // Ideally the buffer would notify the code here when it is empty, but the TZXDuino code is not structured that way.
 
  // Rather than implementing a simple delay here, we count down in the update loop a delay until the buffer should
  // be empty, and then stop the tape.
  // This avoids blocking any thread.
}

/**
 * Re(start) the output timer.
 * 
 * Will be called from the timer ISR, and ZxTape::Update() contexts.
 * 
 * @param periodUs 
 */
void CZxTape::wave_isr_set_period(unsigned long periodUs) {
  m_OutputTimer.Start(periodUs);
}

/**
 * Set the output pin LOW.
 * 
 * Will be called from the timer ISR, and ZxTape::Update() contexts.
 */
void CZxTape::wave_set_low() {
  m_GpioOutputPin.Write(LOW);
}

/**
 * Set the output pin HIGH.
 * 
 * Will be called from the timer ISR, and ZxTape::Update() contexts.
 */
void CZxTape::wave_set_high() {
  m_GpioOutputPin.Write(HIGH);
}

/**
 * The the wave ISR.
 * 
 * Will be called from the timer ISR context.
 * 
 * @param pUserTimer 
 * @param pParam 
 */
void CZxTape::wave_isr(CUserTimer *pUserTimer, void *pParam) {
  wave();
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

  // Disable IRQ
#if ZX_TAPE_USE_FIQ
  EnterCritical(FIQ_LEVEL);
#else
  EnterCritical(IRQ_LEVEL);
#endif    
}


// Enable interrupts
void interrupts() {
  // LOGDBG("interrupts");

  // Reenable Interrupts
  LeaveCritical();
}


// Set the mode of a GPIO pin (i.e. set correct pin to output)
void pinMode(unsigned pin, unsigned mode) {
  LOGDBG("pinMode(%d, %d)", pin, mode);
}


// Set the GPIO output pin low
void LowWrite() {
  // LOGDBG("LowWrite");
  pZxTape->wave_set_low();
}


// Set the GPIO output pin high
void HighWrite() {
  // LOGDBG("HighWrite");
  pZxTape->wave_set_high();
}


// Delay for a number of milliseconds
void delay(unsigned long time) {
  // LOGDBG("delay(%lu)", time);
  // CTimer::Get()->SimpleMsDelay(time);
  // Ignore TXZ delays as they will mess up screen timing.
  // Have to handle in other ways!
}


// End the current file playback (EOF or error)
void stopFile() {
  if (!pZxTape) return;

  LOGDBG("stopFile");

  pZxTape->end_playback();
}


// Called to display the playback time (at start)
void lcdTime() {
  // LOGDBG("lcdTime");
}


// Called to display the playback percent (during playback)
void Counter2() {
  // LOGDBG("Counter2");
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

  pFile = GAME;
  filesize = GAME_SIZE;

  nFileSeekIdx = 0;  

  LOGDBG("filesize: %d", filesize);


  return true;
}

static void filetype_close() {
  LOGDBG("filetype_close");

  pFile = nullptr;
  nFileSeekIdx = 0;
}

static int filetype_read(void* buf, unsigned long count) {
  // LOGDBG("filetype_read(%lu)", count);

  if (nFileSeekIdx + count > filesize) {
    count = filesize - nFileSeekIdx;
  }

  // for (unsigned long i = 0; i < count; i++) {
  //   LOGDBG("%02x ", *(pFile + nFileSeekIdx + i));
  // }

  memcpy(buf, pFile + nFileSeekIdx, count);
  nFileSeekIdx += count;

  return count;
}

static bool filetype_seekSet(uint64_t pos) {
  // LOGDBG("filetype_seekSet(%lu)", pos);

  if (pos >= filesize) return false;

  nFileSeekIdx = pos;

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
  // LOGDBG("timer_initialize");
  // Ignore
}

static void timer_stop() {
  // LOGDBG("timer_stop");
  // Ignore
}

static void timer_setPeriod(unsigned long periodUs) {
  // LOGDBG("timer_setPeriod(%lu)", periodUs);
  pZxTape->wave_isr_set_period(periodUs);
}
