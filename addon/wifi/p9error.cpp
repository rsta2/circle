#include "p9error.h"
#include "p9proc.h"
#include "p9util.h"
#include <assert.h>

static unsigned stackptr = 0;

void error (const char *str)
{
	print ("%s\n", str);

	up->errstr = str;

	assert (stackptr-- > 0);
	assert (0);			// TODO
}

int waserror (void)
{
#ifndef NDEBUG
	stackptr++;
#endif

	return 0;
}

void nexterror (void)
{
	assert (stackptr-- > 0);
	assert (0);			// TODO
}

void poperror (void)
{
	assert (stackptr-- > 0);
}

void okay (int status)
{
}
