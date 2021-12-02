#include <linux/kthread.h>
#include <linux/bug.h>
#include <circle/sched/task.h>
#include <circle/sched/scheduler.h>
#include <circle/string.h>
#include <circle/stdarg.h>

class CKThread : public CTask
{
public:
	CKThread (int (*threadfn) (void *data), void *data, const char *pName)
	:	m_threadfn (threadfn),
		m_data (data)
	{
		SetName (pName);
	}

	void Run (void)
	{
		(*m_threadfn) (m_data);
	}

private:
	int (*m_threadfn) (void *data);
	void *m_data;
};

struct task_struct *current = 0;

static int next_pid = 1;

struct task_struct *kthread_create (int (*threadfn)(void *data),
				    void *data,
				    const char namefmt[], ...)
{
	CString name;
	va_list var;
	va_start (var, namefmt);
	name.FormatV (namefmt, var);
	va_end (var);

	task_struct *task = new task_struct;

	task->pid = next_pid++;
	task->terminated = 0;
	task->userdata = 0;

	CTask *ctask = new CKThread (threadfn, data, name);
	ctask->SetUserData (task, TASK_USER_DATA_KTHREAD);
	task->taskobj = (void *) ctask;

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

static void task_switch_handler (CTask *ctask)
{
	current = (struct task_struct *) ctask->GetUserData (TASK_USER_DATA_KTHREAD);
}

static void task_termination_handler (CTask *ctask)
{
	struct task_struct *task =
		(struct task_struct *) ctask->GetUserData (TASK_USER_DATA_KTHREAD);
	if (task != 0)
	{
		task->terminated = 1;
	}
}

int linuxemu_init_kthread (void)
{
	// init a task_struct for the first kthread, which is not created with kthread_create()
	task_struct *task = new task_struct;

	task->pid = next_pid++;
	task->terminated = 0;
	task->taskobj = 0;
	task->userdata = 0;

	CTask *ctask = CScheduler::Get ()->GetCurrentTask ();
	ctask->SetUserData (task, TASK_USER_DATA_KTHREAD);

	current = task;

	CScheduler::Get ()->RegisterTaskSwitchHandler (task_switch_handler);
	CScheduler::Get ()->RegisterTaskTerminationHandler (task_termination_handler);

	return 0;
}
