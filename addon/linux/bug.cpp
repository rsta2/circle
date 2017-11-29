#include <linux/bug.h>
#include <circle/logger.h>
#include <circle/string.h>

void __warn (const char *file, const int line)
{
	CString Source;
	Source.Format ("%s(%d)", file, line);

	CLogger::Get ()->Write (Source, LogWarning, "WARN_ON failed");
}
