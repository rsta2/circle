//
// logger.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#include <circle/util.h>

#define LOGGER_BUFSIZE	0x4000

CLogger *CLogger::s_pThis = 0;

CLogger::CLogger (unsigned nLogLevel)
:	m_nLogLevel (nLogLevel),
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

	delete m_pBuffer;
	m_pBuffer = 0;

	m_pTarget = 0;
}

boolean CLogger::Initialize (CDevice *pTarget)
{
	m_pTarget = pTarget;
	
	Write ("logger", LogNotice, "Logging started");

	return TRUE;
}

void CLogger::Write (const char *pSource, TLogSeverity Severity, const char *pMessage, ...)
{
	if (Severity > m_nLogLevel)
	{
		return;
	}

	if (Severity == LogPanic)
	{
		Write ("\x1b[1m");
	}

	Write (pSource);
	Write (": ");

	va_list var;
	va_start (var, pMessage);

	CString Message;
	Message.FormatV (pMessage, var);

	Write (Message);

	va_end (var);

	if (Severity == LogPanic)
	{
		Write ("\x1b[0m");
	}

	Write ("\n");

	if (Severity == LogPanic)
	{
		DisableInterrupts ();

		while (1)
		{
			// wait forever
		}
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
}

int CLogger::Read (void *pBuffer, unsigned nCount)
{
	if (m_nInPtr == m_nOutPtr)
	{
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

	return nResult;
}
