#ifndef _p9sd_h
#define _p9sd_h

#include "p9util.h"

typedef struct sdio_t
{
	const char *devname;
	int (*init) (void);
	void (*enable) (void);
	int (*inquiry) (char *buf, int len);
	int (*cmd) (u32int cmd, u32int arg, u32int *resp);
	void (*iosetup) (int write, void *buf, int blksize, int blkcount);
	void (*io)(int write, uchar *buf, int len);
}
SDio;

extern SDio sdio;

#endif
