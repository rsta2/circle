#ifndef _p9chan_h
#define _p9chan_h

#include <fatfs/ff.h>
#include "p9util.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	unsigned type;
	int open;
	FIL file;
	ulong offset;
}
Chan;

Chan *namec (const char *name, unsigned func, unsigned flags, unsigned opt);
#define Aopen		0
#define OREAD		0

void cclose (Chan *chan);

#define devtab	__p9devtab
extern struct device_t
{
	int (*read) (Chan *chan, void *buf, size_t len, ulong off);
}
*devtab[];

void p9chan_init (const char *path);

#ifdef __cplusplus
}
#endif

#endif
