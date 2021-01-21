//
// logger.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef _circle_logger_h
#define _circle_logger_h

#include <circle/device.h>
#include <circle/timer.h>
#include <circle/stdarg.h>
#include <circle/spinlock.h>
#include <circle/time.h>
#include <circle/types.h>

#define LOG_MAX_SOURCE		50
#define LOG_MAX_MESSAGE		200
#define LOG_QUEUE_SIZE		50

enum TLogSeverity
{
	LogPanic,	// Halt the system after processing this message
	LogError,	// Severe error in this component, system may continue to work
	LogWarning,	// Non-severe problem, component continues to work
	LogNotice,	// Informative message, which is interesting for the system user
	LogDebug	// Message, which is only interesting for debugging this component
};

struct TLogEvent;

typedef void TLogEventNotificationHandler (void);
typedef void TLogPanicHandler (void);

class CLogger
{
public:
	CLogger (unsigned nLogLevel, CTimer *pTimer = 0);	// time is not logged if pTimer is 0
	~CLogger (void);

	boolean Initialize (CDevice *pTarget);

	void SetNewTarget (CDevice *pTarget);

	void Write (const char *pSource, TLogSeverity Severity, const char *pMessage, ...)
		__attribute__ ((format (printf, 4, 5)));
	void WriteV (const char *pSource, TLogSeverity Severity, const char *pMessage, va_list Args);

	// does not allocate memory, for critical (low memory) messages
	void WriteNoAlloc (const char *pSource, TLogSeverity Severity, const char *pMessage);

	int Read (void *pBuffer, unsigned nCount);

	// returns FALSE if event is not available
	boolean ReadEvent (TLogSeverity *pSeverity, char *pSource, char *pMessage,
			   time_t *pTime, unsigned *pHundredthTime, int *pTimeZone);

	// handler is called when a log event arrives
	void RegisterEventNotificationHandler (TLogEventNotificationHandler *pHandler);
	// handler is called before the system is halted
	void RegisterPanicHandler (TLogPanicHandler *pHandler);

	static CLogger *Get (void);

private:
	void Write (const char *pString);

	void WriteEvent (const char *pSource, TLogSeverity Severity, const char *pMessage);

private:
	unsigned m_nLogLevel;
	CTimer *m_pTimer;

	CDevice *m_pTarget;

	char *m_pBuffer;
	unsigned m_nInPtr;
	unsigned m_nOutPtr;
	CSpinLock m_SpinLock;

	TLogEvent *m_pEventQueue[LOG_QUEUE_SIZE];
	unsigned m_nEventInPtr;
	unsigned m_nEventOutPtr;
	CSpinLock m_EventSpinLock;

	TLogEventNotificationHandler *m_pEventNotificationHandler;
	TLogPanicHandler *m_pPanicHandler;

	static CLogger *s_pThis;
};

#endif
