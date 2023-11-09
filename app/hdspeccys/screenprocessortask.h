//
// screenprocessortask.h
//
#ifndef _screenprocessortask_h
#define _screenprocessortask_h

#include <circle/sched/task.h>
#include <circle/actled.h>
#include <zxreset/zxreset.h>
#include <zxsmi/zxsmi.h>
#include <zxscreen/zxscreen.h>
 
class CScreenProcessorTask : public CTask
{
public:
	CScreenProcessorTask (CZxReset *pZxReset, CZxScreen *pZxScreen, CZxSmi *pZxSmi, CActLED *pActLED);
	~CScreenProcessorTask (void);

	void Run (void);

private:
	CSynchronizationEvent	m_FrameEvent;

	CZxReset		*m_pZxReset;
	CZxScreen		*m_pZxScreen;
	CZxSmi			*m_pZxSmi;	
	CActLED			*m_pActLED;	

	volatile ZX_DMA_T *m_pScreenPixelDataBuffer;
	volatile ZX_DMA_T *m_pScreenAttrDataBuffer;
	u32 m_nScreenDataBufferLength;
};

#endif // #ifndef _screenprocessortask_h
