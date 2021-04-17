//
/// \file logger.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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

#define LOGGER_BUFSIZE		0x4000		///< Size of the text ring buffer

enum TLogSeverity
{
	LogPanic,	///< Halt the system after processing this message
	LogError,	///< Severe error in this component, system may continue to work
	LogWarning,	///< Non-severe problem, component continues to work
	LogNotice,	///< Informative message, which is interesting for the system user
	LogDebug	///< Message, which is only interesting for debugging this component
};

struct TLogEvent;

typedef void TLogEventNotificationHandler (void);
typedef void TLogPanicHandler (void);

class CLogger		/// Writing logging messages to a target device
{
public:
	/// \param nLogLevel Only messages with (Severity <= nLogLevel) will be logged
	/// \param pTimer Pointer to system timer object (time is not logged, if this is 0)
	/// \param bOverwriteOldest Overwrite oldest text in text ring buffer, if it is full?
	CLogger (unsigned nLogLevel, CTimer *pTimer = 0, boolean bOverwriteOldest = TRUE);

	~CLogger (void);

	/// \param pTarget Pointer to target device
	boolean Initialize (CDevice *pTarget);

	/// \param pTarget Pointer to new target device
	void SetNewTarget (CDevice *pTarget);

	/// \param pSource  Module name of the originator of the log message
	/// \param Severity Severity of the log message
	/// \param pMessage Format string of the log message (arguments follow)
	void Write (const char *pSource, TLogSeverity Severity, const char *pMessage, ...);

	/// \param pSource  Module name of the originator of the log message
	/// \param Severity Severity of the log message
	/// \param pMessage Format string of the log message
	/// \param Args	    Arguments of the log message
	void WriteV (const char *pSource, TLogSeverity Severity, const char *pMessage, va_list Args);

	/// \brief Does not allocate memory, for critical (low memory) messages
	void WriteNoAlloc (const char *pSource, TLogSeverity Severity, const char *pMessage);

	/// \brief Read log message text from the log text ring buffer
	/// \param pBuffer Read text is copied to this buffer
	/// \param nCount  Size of the buffer
	/// \param bClear  Remove the returned bytes from buffer
	/// \return Number of bytes copied to the buffer (0 for none)
	int Read (void *pBuffer, unsigned nCount, boolean bClear = TRUE);

	/// \brief Read the next log event from the log event ring buffer
	/// \param pSeverity	  Severity is returned here
	/// \param pSource	  Source module name is returned here (size LOG_MAX_SOURCE)
	/// \param pMessage	  Message string is returned here (size LOG_MAX_MESSAGE)
	/// \param pTime	  Time is returned here
	/// \param pHundredthTime 1/100s part of the time is returned here
	/// \param pTimeZone	  Timezone is returned here
	/// \return FALSE if event is not available
	boolean ReadEvent (TLogSeverity *pSeverity, char *pSource, char *pMessage,
			   time_t *pTime, unsigned *pHundredthTime, int *pTimeZone);

	/// \brief Register handler which is called, when a log event arrives
	void RegisterEventNotificationHandler (TLogEventNotificationHandler *pHandler);
	/// \brief Register handler which is called, before the system is halted
	void RegisterPanicHandler (TLogPanicHandler *pHandler);

	/// \return Pointer to the (only) system object of CLogger
	/// \note If no instance of CLogger exists, a dummy instance is created
	static CLogger *Get (void);

private:
	void Write (const char *pString);

	void WriteEvent (const char *pSource, TLogSeverity Severity, const char *pMessage);

private:
	unsigned m_nLogLevel;
	CTimer *m_pTimer;
	boolean m_bOverwriteOldest;

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

/// Define name for this module, for generating log messages
#define LOGMODULE(name)		static const char From[] = name

#define LOGPANIC(...)		CLogger::Get ()->Write (From, LogPanic, __VA_ARGS__)
#define LOGERR(...)		CLogger::Get ()->Write (From, LogError, __VA_ARGS__)
#define LOGWARN(...)		CLogger::Get ()->Write (From, LogWarning, __VA_ARGS__)
#define LOGNOTE(...)		CLogger::Get ()->Write (From, LogNotice, __VA_ARGS__)
#define LOGDBG(...)		CLogger::Get ()->Write (From, LogDebug, __VA_ARGS__)

#endif
