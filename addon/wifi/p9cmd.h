#ifndef _p9cmd_h
#define _p9cmd_h

#include "p9util.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	unsigned index;
	const char *cmd;
	unsigned maxargs;
}
Cmdtab;

typedef struct
{
	char buf[200];
	unsigned argc;
	char *f[10];
}
Cmdbuf;

void cmderror (Cmdbuf *cb, const char *err);
Cmdbuf *parsecmd (const void *str, long n);
Cmdtab *lookupcmd (Cmdbuf *cb, Cmdtab *ct, size_t nelem);

#ifdef __cplusplus
}
#endif

#endif
