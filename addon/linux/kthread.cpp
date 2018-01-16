#include <linux/kthread.h>
#include <circle/sched/task.h>

class CKThread : public CTask
{
public:
	CKThread (int (*threadfn) (void *data), void *data)
	:	m_threadfn (threadfn),
		m_data (data)
	{
	}

	void Run (void)
	{
		(*m_threadfn) (m_data);
	}

private:
	int (*m_threadfn) (void *data);
	void *m_data;
};

// TODO: current is not set by the scheduler
struct task_struct *current = 0;

static int next_pid = 1;

struct task_struct *kthread_create (int (*threadfn)(void *data),
				    void *data,
				    const char namefmt[], ...)
{
	task_struct *task = new task_struct;

	task->pid = next_pid++;

	task->taskobj = (void *) new CKThread (threadfn, data);

	return task;
}

void set_user_nice (struct task_struct *task, long nice)
{
}

// TODO: task is waken by default
int wake_up_process (struct task_struct *task)
{
	return 0;
}

void flush_signals (struct task_struct *task)
{
}
