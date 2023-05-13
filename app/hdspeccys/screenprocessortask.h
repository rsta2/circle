//
// screenprocessortask.h
//
#ifndef _screenprocessortask_h
#define _screenprocessortask_h

#include <circle/sched/task.h>
#include <circle/actled.h>
#include <zxsmi/zxsmi.h>
#include <zxscreen/zxscreen.h>
 
class CScreenProcessorTask : public CTask
{
public:
	CScreenProcessorTask (CZxScreen *pZxScreen, CZxSmi *pZxSmi, CActLED *pActLED);
	~CScreenProcessorTask (void);

	void Run (void);

private:
	CSynchronizationEvent	m_FrameEvent;

	CZxScreen		*m_pZxScreen;
	CZxSmi			*m_pZxSmi;	
	CActLED			*m_pActLED;	
};

#endif
