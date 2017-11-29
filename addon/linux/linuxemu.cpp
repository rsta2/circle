#include <linux/linuxemu.h>
#include <linux/timer.h>

int linuxemu_init (void)
{
	static int init_done = 0;
	if (init_done)
	{
		return 0;
	}
	init_done = 1;

	int ret = linuxemu_init_timer ();
	if (ret != 0)
	{
		return ret;
	}

	return 0;
}
