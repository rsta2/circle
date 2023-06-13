//
// zxtape.h
//
#ifndef _zxtape_h
#define _zxtape_h

#include <circle/types.h>
#include <circle/interrupt.h>
#include <circle/logger.h>

class CZxTape
{
public:
	CZxTape (CInterruptSystem *pInterruptSystem);
	~CZxTape (void);

	// methods ...
	boolean Initialize(void);
	void PlayPause(void);
	void Stop(void);
	void Update(void);

private:
	void loop_playback(void);
	void loop_control(void);
	bool check_button_play_pause(void);
	bool check_button_stop(void);
	void play_file(void);
	void stop_file(void);	

private:
	// members ...
	CInterruptSystem *m_pInterruptSystem;
	
	bool m_bRunning;
	bool m_bPauseOn;
	bool m_bButtonPlayPause;
	bool m_bButtonStop;
};

#endif // _zxtape_h
