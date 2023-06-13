//
// zxtape.cpp
//

#include "zxtape.h"
#include "tzx.h"
#include "chuckie-egg.h"


LOGMODULE ("ZxTape");


/* Definitions */
// NONE



//
// Class
//

CZxTape::CZxTape (CInterruptSystem *pInterruptSystem)
: m_pInterruptSystem (pInterruptSystem),
  m_bRunning (false),
  m_bPauseOn (false),
	m_bButtonPlayPause (false),  
	m_bButtonStop (false)
{

}

CZxTape::~CZxTape (void)
{
  // NOTHING TO DO
}

/**
 * Initialise ZX Tape
 */
boolean CZxTape::Initialize ()
{
  // unsigned i;
  // unsigned dmaBufferLenWords = ZX_DMA_BUFFER_LENGTH;
  // unsigned dmaBufferLenBytes = ZX_DMA_BUFFER_LENGTH * sizeof(ZX_DMA_T);
  
  LOGNOTE("Initializing ZX TAPE");

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


