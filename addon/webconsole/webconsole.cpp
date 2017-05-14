//
// webconsole.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
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
#include <webconsole/webconsole.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <assert.h>

#define LOG_BUFFER_SIZE		20000

static const char s_Header[] = "<pre>\n";

CWebConsole::CWebConsole (CNetSubSystem *pNetSubSystem, u16 nPort, CSocket *pSocket, CLogBuffer *pLog)
:	CHTTPDaemon (pNetSubSystem, pSocket, LOG_BUFFER_SIZE + sizeof s_Header-1, nPort),
	m_nPort (nPort),
	m_pLog (pLog),
	m_bLogCreated (FALSE)
{
	if (m_pLog == 0)
	{
		m_pLog = new CLogBuffer (LOG_BUFFER_SIZE);
		assert (m_pLog != 0);

		m_bLogCreated = TRUE;
	}
}

CWebConsole::~CWebConsole (void)
{
	if (m_bLogCreated)
	{
		delete m_pLog;
		m_pLog = 0;
	}
}

CHTTPDaemon *CWebConsole::CreateWorker (CNetSubSystem *pNetSubSystem, CSocket *pSocket)
{
	return new CWebConsole (pNetSubSystem, m_nPort, pSocket, m_pLog);
}

THTTPStatus CWebConsole::GetContent (const char  *pPath,
				     const char  *pParams,
				     const char  *pFormData,
				     u8	         *pBuffer,
				     unsigned    *pLength,
				     const char **ppContentType)
{
	assert (m_pLog != 0);

	assert (pPath != 0);
	if (   strcmp (pPath, "/") != 0
	    && strcmp (pPath, "/index.html") != 0)
	{
		return HTTPNotFound;
	}

	assert (pBuffer != 0);
	memcpy (pBuffer, s_Header, sizeof s_Header);
	pBuffer += sizeof s_Header;

	char Buffer[200];
	int nBytesRead;
	while ((nBytesRead = CLogger::Get ()->Read (Buffer, sizeof Buffer)) > 0)
	{
		m_pLog->Put (Buffer, nBytesRead);
	}

	unsigned nLength = sizeof s_Header + m_pLog->Get (pBuffer);

	assert (pLength != 0);
	assert (*pLength >= nLength);
	*pLength = nLength;

	assert (ppContentType != 0);
	*ppContentType = "text/html; charset=iso-8859-1";

	return HTTPOK;
}
