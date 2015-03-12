//
// logger.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2015  R. Stange <rsta2@o2online.de>
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
#ifndef _logger_h
#define _logger_h

#include <circle/device.h>
#include <circle/timer.h>
#include <circle/stdarg.h>
#include <circle/spinlock.h>
#include <circle/types.h>

enum TLogSeverity
{
	LogPanic,
	LogError,
	LogWarning,
	LogNotice,
	LogDebug
};

class CLogger
{
public:
	CLogger (unsigned nLogLevel, CTimer *pTimer = 0);	// time is not logged if pTimer is 0
	~CLogger (void);

	boolean Initialize (CDevice *pTarget);

	void Write (const char *pSource, TLogSeverity Severity, const char *pMessage, ...);
	void WriteV (const char *pSource, TLogSeverity Severity, const char *pMessage, va_list Args);

	int Read (void *pBuffer, unsigned nCount);

	static CLogger *Get (void);

private:
	void Write (const char *pString);

private:
	unsigned m_nLogLevel;
	CTimer *m_pTimer;

	CDevice *m_pTarget;

	char *m_pBuffer;
	unsigned m_nInPtr;
	unsigned m_nOutPtr;
	CSpinLock m_SpinLock;

	static CLogger *s_pThis;
};

#endif
