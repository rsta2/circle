//
// tapetask.h
//
#ifndef _tapetask_h
#define _tapetask_h

#include <circle/sched/task.h>
#include <zxtape/zxtape.h>
 
class CTapeTask : public CTask
{
public:
	CTapeTask (CZxTape *pZxTape);
	~CTapeTask (void);

	void Run (void);

private:
	CZxTape		*m_pZxTape;
};

#endif // #ifndef _tapetask_h
