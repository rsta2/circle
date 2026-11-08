#pragma once
[[noreturn]] void ModelPanic (const char *format, ...);
#define LOGMODULE(x)
#define LOGPANIC(...) ModelPanic (__VA_ARGS__)
#define LOGWARN(...) ((void)0)
#define LOGTRACE(...) ((void)0)
enum
{
	LogTrace
};
class CLogger
{
      public:
	static CLogger *Get ()
	{
		static CLogger log;
		return &log;
	}
	void Write (const char *, int, const char *, ...)
	{
	}
};
