//
// backgroundtask.h
//
#ifndef _backgroundtask_h
#define _backgroundtask_h

#include <circle/sched/task.h>
#include <circle/sched/synchronizationevent.h>
#include <circle/actled.h>
#include <shell/shell.h>
 
class CBackgroundTask : public CTask
{
public:
	CBackgroundTask (CShell *pShell, CActLED *pActLED, CSynchronizationEvent *pResetEvent);
	~CBackgroundTask (void);

	void Run (void);

private:
	// Callbacks
	static void Reboot (void* pContext);

private:
	CShell						*m_pShell;
	CActLED						*m_pActLED;
	CSynchronizationEvent		*m_pRebootEvent;
};

#endif
