#include <linux/kernel.h>
#include <circle/string.h>
#include <circle/stdarg.h>
#include <circle/util.h>

#if STDLIB_SUPPORT <= 1

int sprintf (char *buf, const char *fmt, ...)
{
	va_list var;
	va_start (var, fmt);

	CString Msg;
	Msg.FormatV (fmt, var);

	va_end (var);

	strcpy (buf, (const char *) Msg);

	return Msg.GetLength ();
}

int snprintf (char *buf, size_t size, const char *fmt, ...)
{
	va_list var;
	va_start (var, fmt);

	CString Msg;
	Msg.FormatV (fmt, var);

	va_end (var);

	size_t len = Msg.GetLength ();
	if (--size < len)
	{
		len = size;
	}

	memcpy (buf, (const char *) Msg, len);
	buf[len] = '\0';

	return len;
}

int vsnprintf (char *buf, size_t size, const char *fmt, va_list var)
{
	CString Msg;
	Msg.FormatV (fmt, var);

	size_t len = Msg.GetLength ();
	if (--size < len)
	{
		len = size;
	}

	memcpy (buf, (const char *) Msg, len);
	buf[len] = '\0';

	return len;
}

#endif
