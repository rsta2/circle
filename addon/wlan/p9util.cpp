#include "p9util.h"
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/stdarg.h>
#include <circle/debug.h>
#include <assert.h>

static const char From[] = "wlan";

int print (const char *fmt, ...)
{
	va_list var;
	va_start (var, fmt);

	CString Msg;
	Msg.FormatV (fmt, var);

	va_end (var);

	char Buffer[1000];
	strncpy (Buffer, (const char *) Msg, sizeof Buffer);
	Buffer[sizeof Buffer-1] = '\0';

	char *pLine;
	char *pLineEnd;
	for (pLine = Buffer; (pLineEnd = strchr (pLine, '\n')) != 0; pLine = pLineEnd+1)
	{
		*pLineEnd = '\0';

		CLogger::Get ()->Write (From, LogDebug, "%s", pLine);
	}

	if (*pLine != '\0')
	{
		CLogger::Get ()->Write (From, LogDebug, "%s", pLine);
	}

	return Msg.GetLength ();
}

int sprint (char *str, const char *fmt, ...)
{
	va_list var;
	va_start (var, fmt);

	CString Msg;
	Msg.FormatV (fmt, var);

	va_end (var);

	strcpy (str, (const char *) Msg);

	return Msg.GetLength ();
}

int snprint (char *str, size_t size, const char *fmt, ...)
{
	va_list var;
	va_start (var, fmt);

	CString Msg;
	Msg.FormatV (fmt, var);

	va_end (var);

	strncpy (str, (const char *) Msg, size);
	str[size-1] = '\0';

	return Msg.GetLength () < size-1 ? Msg.GetLength () : size-1;
}

char *seprint (char *str, char *end, const char *fmt, ...)
{
	va_list var;
	va_start (var, fmt);

	CString Msg;
	Msg.FormatV (fmt, var);

	va_end (var);

	assert (str < end);
	size_t len = end-str-1;

	strncpy (str, (const char *) Msg, len);
	*(end-1) = '\0';

	return str + (Msg.GetLength () < len ? Msg.GetLength () : len);
}

long readstr (ulong offset, void *buf, size_t len, const void *p)
{
	const char *p1 = (const char *) p;

	size_t plen = strlen (p1);
	if (offset >= plen)
	{
		return 0;
	}

	p1 += offset;

	char *p2 = (char *) buf;
	long result = 0;

	while (*p1 != '\0' && len > 0)
	{
		*p2++ = *p1++;

		result++;
		len--;
	}

	return result;
}

void hexdump (const void *p, size_t len, const char *from)
{
#ifndef NDEBUG
	debug_hexdump (p, len, from);
#endif
}
