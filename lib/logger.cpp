//
// logger.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/synchronize.h>
#include <circle/startup.h>
#include <circle/multicore.h>
#include <circle/util.h>
#include <circle/sysconfig.h>
#include <circle/machineinfo.h>
#include <circle/version.h>
#include <circle/debug.h>

#define LOGGER_BUFSIZE	0x4000

CLogger *CLogger::s_pThis = 0;

CLogger::CLogger (unsigned nLogLevel, CTimer *pTimer)
:	m_nLogLevel (nLogLevel),
	m_pTimer (pTimer),
	m_pTarget (0),
	m_pBuffer (0),
	m_nInPtr (0),
	m_nOutPtr (0)
{
	m_pBuffer = new char[LOGGER_BUFSIZE];

	s_pThis = this;
}

CLogger::~CLogger ()
{
	s_pThis = 0;

	delete [] m_pBuffer;
	m_pBuffer = 0;

	m_pTarget = 0;
	m_pTimer = 0;
}

boolean CLogger::Initialize (CDevice *pTarget)
{
	m_pTarget = pTarget;

	Write ("logger", LogNotice, CIRCLE_NAME " " CIRCLE_VERSION_STRING " started on %s",
	       CMachineInfo::Get ()->GetMachineName ());

	return TRUE;
}

void CLogger::Write (const char *pSource, TLogSeverity Severity, const char *pMessage, ...)
{
	va_list var;
	va_start (var, pMessage);

	WriteV (pSource, Severity, pMessage, var);

	va_end (var);
}

void CLogger::WriteV (const char *pSource, TLogSeverity Severity, const char *pMessage, va_list Args)
{
	if (Severity > m_nLogLevel)
	{
		return;
	}

	CString Buffer;

	if (Severity == LogPanic)
	{
		Buffer = "\x1b[1m";
	}

	if (m_pTimer != 0)
	{
		CString *pTimeString = m_pTimer->GetTimeString ();
		if (pTimeString != 0)
		{
			Buffer.Append (*pTimeString);
			Buffer.Append (" ");

			delete pTimeString;
		}
	}

	Buffer.Append (pSource);
	Buffer.Append (": ");

	CString Message;
	Message.FormatV (pMessage, Args);

	Buffer.Append (Message);

	if (Severity == LogPanic)
	{
		Buffer.Append ("\x1b[0m");
	}

	Buffer.Append ("\n");

	Write (Buffer);

	if (Severity == LogPanic)
	{
#ifndef USE_RPI_STUB_AT
#ifndef ARM_ALLOW_MULTI_CORE
		halt ();
#else
		CMultiCoreSupport::HaltAll ();
#endif
#else
		Breakpoint (0);
#endif
	}
}

CLogger *CLogger::Get (void)
{
	return s_pThis;
}

void CLogger::Write (const char *pString)
{
	unsigned long nLength = strlen (pString);

	m_pTarget->Write (pString, nLength);

	m_SpinLock.Acquire ();

	while (nLength--)
	{
		char c = *pString++;
		if (c == '\r')
		{
			continue;
		}

		m_pBuffer[m_nInPtr] = c;

		m_nInPtr = (m_nInPtr + 1) % LOGGER_BUFSIZE;

		if (m_nInPtr == m_nOutPtr)
		{
			m_nInPtr = (m_nInPtr - 1) % LOGGER_BUFSIZE;

			break;
		}
	}

	m_SpinLock.Release ();
}

int CLogger::Read (void *pBuffer, unsigned nCount)
{
	m_SpinLock.Acquire ();

	if (m_nInPtr == m_nOutPtr)
	{
		m_SpinLock.Release ();

		return -1;
	}

	char *pchBuffer = (char *) pBuffer;
	int nResult = 0;

	while (nCount--)
	{
		*pchBuffer++ = m_pBuffer[m_nOutPtr];

		m_nOutPtr = (m_nOutPtr + 1) % LOGGER_BUFSIZE;

		nResult++;

		if (m_nInPtr == m_nOutPtr)
		{
			break;
		}
	}

	m_SpinLock.Release ();

	return nResult;
}
