#include <linux/printk.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/stdarg.h>
#include <circle/util.h>

#define PRINTK_MAX_MSG_LEN	1000

static const char From[] = "printk";

int printk (const char *fmt, ...)
{
	va_list var;
	va_start (var, fmt);

	CString Msg;
	Msg.FormatV (fmt, var);

	va_end (var);

	char Buffer[PRINTK_MAX_MSG_LEN+1];
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
