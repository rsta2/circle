//
// zxtape.h
//
#ifndef _zxtape_h
#define _zxtape_h

#include <circle/types.h>
#include <circle/interrupt.h>
#include <circle/usertimer.h>
#include <circle/gpiomanager.h>
#include <circle/gpiopin.h>
#include <circle/gpiopinfiq.h>
#include <circle/logger.h>

#define ZX_TAPE_GPIO_OUTPUT_PIN				0	// GPIO 26 (HW PIN 37), GPIO 0 (HW PIN 27)
#define ZX_TAPE_USE_FIQ						FALSE
// #define ZX_TAPE_USE_FIQ					TRUE

// Maximum length for long filename support (ideally as large as possible to support very long filenames)
#define ZX_TAPE_MAX_FILENAME_LEN   			1023      		

class CZxTape
{
public:
	CZxTape (CGPIOManager *pGPIOManager, CInterruptSystem *pInterruptSystem);
	~CZxTape (void);

	// methods ...
	boolean Initialize(void);
	void PlayPause(void);
	void Stop(void);
	bool IsStarted(void);
	bool IsPlaying(void);
	bool IsPaused(void);
	bool IsStopped(void);
	void Update(void);

private:
	void loop_playback(void);
	void loop_control(void);
	bool check_button_play_pause(void);
	bool check_button_stop(void);
	void play_file(void);
public:
	// Do not call these methods directly, they are called internally from C code
	void stop_file(void);	
	void wave_isr_set_period(unsigned long periodUs);
	void wave_set_low();
	void wave_set_high();
private:
	static void wave_isr(CUserTimer *pUserTimer, void *pParam);	

private:
	// members ...
	CGPIOManager *m_pGPIOManager;
	CInterruptSystem *m_pInterruptSystem;	
	CGPIOPin	m_GpioOutputPin;
	CUserTimer m_OutputTimer;

	bool m_bRunning;
	bool m_bButtonPlayPause;
	bool m_bButtonStop;

	unsigned m_nLastControlTicks;
};

#endif // _zxtape_h
