#include "p9cmd.h"
#include "p9error.h"
#include <circle/string.h>
#include <assert.h>

void cmderror (Cmdbuf *cb, const char *err)
{
	if (cb->argc > 0)
	{
		CString Cmd;
		for (unsigned i = 0; i < cb->argc; i++)
		{
			if (i > 0)
			{
				Cmd.Append (" ");
			}

			Cmd.Append (cb->f[i]);
		}

		print ("%s\n", (const char *) Cmd);
	}

	error (err);
}

Cmdbuf *parsecmd (const void *str, long n)
{
	Cmdbuf *pCmdbuf = (Cmdbuf *) malloc (sizeof (Cmdbuf));
	assert (pCmdbuf != 0);

	strncpy (pCmdbuf->buf, (const char *) str, sizeof pCmdbuf->buf-1);
	pCmdbuf->buf[sizeof pCmdbuf->buf-1] = '\0';

	pCmdbuf->argc = 0;

	char *pSavePtr;
	for (unsigned i = 0; i < nelem (pCmdbuf->f); i++)
	{
		char *p = strtok_r (i == 0 ? pCmdbuf->buf : 0, " \t\n", &pSavePtr);
		pCmdbuf->f[i] = p;
		if (p == 0)
		{
			break;
		}

		CString Arg (p);
		if (Arg.Replace ("\\x20", " ") > 0)
		{
			strcpy (p, Arg);
		}

		pCmdbuf->argc++;
	}

	return pCmdbuf;
}

Cmdtab *lookupcmd (Cmdbuf *cb, Cmdtab *ct, size_t nelem)
{
	if (cb->argc == 0)
	{
		cmderror (cb, "Command expected");
	}

	for (unsigned i = 0; i < nelem; i++, ct++)
	{
		if (strcasecmp (cb->f[0], ct->cmd) == 0)
		{
			return ct;
		}
	}

	cmderror (cb, "Invalid command");

	return 0;
}
