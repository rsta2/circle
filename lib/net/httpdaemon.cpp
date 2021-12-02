//
// httpdaemon.cpp
//
// A simple HTTP webserver
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/net/httpdaemon.h>
#include <circle/net/in.h>
#include <circle/netdevice.h>
#include <circle/sysconfig.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/util.h>
#include <assert.h>

#define HTTPD_VERSION		"0.02"
#define SERVER			"CHTTPDaemon/" HTTPD_VERSION " (Circle)"

#define MAX_CLIENTS		10

#define HTTPD_STACK_SIZE	TASK_STACK_SIZE

static const char FromHTTPDaemon[] = "httpd";

unsigned CHTTPDaemon::s_nInstanceCount = 0;

CHTTPDaemon::CHTTPDaemon (CNetSubSystem *pNetSubSystem, CSocket *pSocket,
			  unsigned nMaxContentSize, u16 nPort, unsigned nMaxMultipartSize)
:	CTask (HTTPD_STACK_SIZE),
	m_pNetSubSystem (pNetSubSystem),
	m_pSocket (pSocket),
	m_nMaxContentSize (nMaxContentSize),
	m_nPort (nPort),
	m_nMaxMultipartSize (nMaxMultipartSize),
	m_pContentBuffer (0)
{
	s_nInstanceCount++;

	if (m_nMaxContentSize > 0)
	{
		m_pContentBuffer = new u8[m_nMaxContentSize];
		assert (m_pContentBuffer != 0);
	}

	if (pSocket == 0)
	{
		SetName (FromHTTPDaemon);
	}
	else
	{
		CString TaskName;
		TaskName.Format ("httpd@%lp", this);

		SetName (TaskName);
	}
}

CHTTPDaemon::~CHTTPDaemon (void)
{
	assert (m_pSocket == 0);

	delete m_pContentBuffer;
	m_pContentBuffer = 0;

	m_pNetSubSystem = 0;

	s_nInstanceCount--;
}

void CHTTPDaemon::Run (void)
{
	if (m_pSocket == 0)
	{
		Listener ();
	}
	else
	{
		Worker ();
	}
}

void CHTTPDaemon::WriteAccessLog (const CIPAddress &rRemoteIP, THTTPRequestMethod RequestMethod,
				  const char *pRequestURI, THTTPStatus Status,
				  unsigned nContentLength)
{
	assert (pRequestURI != 0);

	CString IPString;
	rRemoteIP.Format (&IPString);

	const char *pMethod;
	switch (RequestMethod)
	{
	case HTTPRequestMethodGet:	pMethod = "GET";	break;
	case HTTPRequestMethodHead:	pMethod = "HEAD";	break;
	case HTTPRequestMethodPost:	pMethod = "POST";	break;
	default:			pMethod = "UNKNOWN";	break;
	}

	CLogger::Get ()->Write (FromHTTPDaemon, LogDebug, "%s \"%s %s\" %u %u",
				(const char *) IPString, pMethod, pRequestURI, Status, nContentLength);
}

void CHTTPDaemon::Listener (void)
{
	assert (m_pNetSubSystem != 0);
	m_pSocket = new CSocket (m_pNetSubSystem, IPPROTO_TCP);
	assert (m_pSocket != 0);

	if (m_pSocket->Bind (m_nPort) < 0)
	{
		CLogger::Get ()->Write (FromHTTPDaemon, LogError, "Cannot bind socket (port %u)", m_nPort);

		delete m_pSocket;
		m_pSocket = 0;

		return;
	}

	if (m_pSocket->Listen (MAX_CLIENTS) < 0)
	{
		CLogger::Get ()->Write (FromHTTPDaemon, LogError, "Cannot listen on socket");

		delete m_pSocket;
		m_pSocket = 0;

		return;
	}

	while (1)
	{
		CIPAddress ForeignIP;
		u16 nForeignPort;
		CSocket *pConnection = m_pSocket->Accept (&ForeignIP, &nForeignPort);
		if (pConnection == 0)
		{
			CLogger::Get ()->Write (FromHTTPDaemon, LogWarning, "Cannot accept connection");

			continue;
		}

		if (s_nInstanceCount >= MAX_CLIENTS+1)
		{
			CLogger::Get ()->Write (FromHTTPDaemon, LogWarning, "Too many clients");

			delete pConnection;

			continue;
		}

		CreateWorker (m_pNetSubSystem, pConnection);
	}
}

void CHTTPDaemon::Worker (void)
{
	assert (m_pSocket != 0);

	// parse HTTP request
	THTTPStatus Status = ParseRequest ();
	if (Status == HTTPUnknownError)		// unknown error cannot be reported to client
	{
		delete m_pSocket;
		m_pSocket = 0;

		return;
	}

	// process HTTP request
	unsigned nContentLength = m_nMaxContentSize;
	const char *pContentType = "text/html";

	const char *pStatusMsg = "OK";

	if (Status == HTTPOK)
	{
		// get content
		assert (m_pContentBuffer != 0);
		Status = GetContent (m_RequestPath, m_RequestParams, m_RequestFormData,
				     m_pContentBuffer, &nContentLength, &pContentType);
		assert (nContentLength <= m_nMaxContentSize);
		assert (pContentType != 0);

		delete [] m_pMultipartBuffer;
		m_pMultipartBuffer = 0;
	}

	if (Status != HTTPOK)
	{
		switch (Status)
		{
		case HTTPBadRequest:		pStatusMsg = "Bad Request";			break;
		case HTTPNotFound:		pStatusMsg = "Not Found";			break;
		case HTTPRequestEntityTooLarge:	pStatusMsg = "Request Entity Too Large";	break;
		case HTTPRequestURITooLong:	pStatusMsg = "Request-URI Too Long";		break;
		case HTTPInternalServerError:	pStatusMsg = "Internal Server Error";		break;
		case HTTPMethodNotImplemented:	pStatusMsg = "Method Not Implemented";		break;
		case HTTPVersionNotSupported:	pStatusMsg = "Version Not Supported";		break;
		default:			pStatusMsg = "Unknown Error";			break;
		}

		CString ErrorPage;
		ErrorPage.Format ("<!DOCTYPE html>\n"
				  "<html>\n"
				  "<head><title>%u %s</title></head>\n"
				  "<body><h1>%s</h1></body>\n"
				  "</html>\n", Status, pStatusMsg, pStatusMsg);

		nContentLength = ErrorPage.GetLength ();
		if (nContentLength > m_nMaxContentSize)
		{
			nContentLength = m_nMaxContentSize;
		}

		assert (m_pContentBuffer != 0);
		memcpy (m_pContentBuffer, (const char *) ErrorPage, nContentLength);
		pContentType = "text/html";	// may has been changed by GetContent()
	}

	// write access log
	const u8 *pClientIP = m_pSocket->GetForeignIP ();
	if (pClientIP == 0)			// connection closed in the meantime?
	{
		delete m_pSocket;
		m_pSocket = 0;

		return;
	}
	CIPAddress ClientIP (pClientIP);

	WriteAccessLog (ClientIP, m_RequestMethod, m_RequestURI, Status, nContentLength);

	// send HTTP response header
	CString Header;
	Header.Format ("HTTP/1.1 %u %s\r\n"
		       "Server: " SERVER "\r\n"
		       "Content-Type: %s\r\n"
		       "Content-Length: %u\r\n"
		       "Connection: close\r\n"
		       "\r\n", Status, pStatusMsg, pContentType, nContentLength);

	if (m_pSocket->Send ((const char *) Header, Header.GetLength (), MSG_DONTWAIT) < 0)
	{
		CLogger::Get ()->Write (FromHTTPDaemon, LogError, "Cannot send response header");

		delete m_pSocket;
		m_pSocket = 0;

		return;
	}

	// send response
	if (   m_RequestMethod != HTTPRequestMethodHead
	    && nContentLength > 0)
	{
		assert (m_pContentBuffer != 0);
		if (m_pSocket->Send (m_pContentBuffer, nContentLength, MSG_DONTWAIT) < 0)
		{
			CLogger::Get ()->Write (FromHTTPDaemon, LogError, "Cannot send response");

			delete m_pSocket;
			m_pSocket = 0;

			return;
		}
	}

	delete m_pSocket;		// closes connection
	m_pSocket = 0;
}

THTTPStatus CHTTPDaemon::ParseRequest (void)
{
	THTTPStatus Status = HTTPOK;

	m_RequestMethod = HTTPRequestMethodUnknown;
	m_RequestURI[0] = '\0';
	m_RequestPath[0] = '\0';
	m_RequestParams[0] = '\0';
	m_bRequestFormDataAvailable = FALSE;
	m_nRequestContentLength = 0;
	m_RequestFormData[0] = '\0';
	m_bMultipartFormDataAvailable = FALSE;
	m_MultipartBoundary[0] = '\0';
	m_nMultipartContentLength = 0;
	m_pMultipartBuffer = 0;

	char Buffer[FRAME_BUFFER_SIZE];
	char Line[HTTP_MAX_REQUEST_LINE+1];
#if HTTP_MAX_REQUEST_LINE+FRAME_BUFFER_SIZE+2000 > HTTPD_STACK_SIZE
	#error Increase HTTPD_STACK_SIZE!
#endif

	unsigned nState = 0; // 0: parse header, 1: parse form data, 2: parse multipart data, 3: leave
	unsigned nLine = 0;
	unsigned nChar = 0;

	int nResult;

	assert (m_pSocket != 0);
	while (   nState < 3
	       && (nResult = m_pSocket->Receive (Buffer, sizeof Buffer, 0)) > 0)
	{
		for (unsigned i = 0; i < (unsigned) nResult; i++)
		{
			char chChar = Buffer[i];

			if (nState == 0)
			{
				if (chChar == '\r')
				{
					continue;
				}

				if (chChar == '\n')		// end of line
				{
					if (nChar == 0)		// empty line is end of header
					{
						if (   m_bRequestFormDataAvailable
						    && m_nRequestContentLength > 0)
						{
							if (m_nRequestContentLength <= HTTP_MAX_FORM_DATA)
							{
								nChar = 0;
								nState = 1;
							}
							else
							{
								Status = HTTPRequestEntityTooLarge;
								nState = 3;
							}
						}
						else if (   m_bMultipartFormDataAvailable
							 && m_nRequestContentLength > 0)
						{
							m_nMultipartContentLength = m_nRequestContentLength;
							m_nRequestContentLength = 0;

							if (m_nMultipartContentLength <= m_nMaxMultipartSize)
							{
								assert (m_pMultipartBuffer == 0);
								m_pMultipartBuffer = new char[m_nMultipartContentLength];
								if (m_pMultipartBuffer == 0)
								{
									Status = HTTPInternalServerError;
									nState = 3;
								}
								else
								{
									nChar = 0;
									nState = 2;
								}
							}
							else
							{
								Status = HTTPRequestEntityTooLarge;
								nState = 3;
							}
						}
						else
						{
							nState = 3;
						}
					}
					else
					{
						if (nLine++ == 0)	// first line?
						{
							if (Status == HTTPOK)
							{						
								Status = ParseMethod (Line);
							}
						}
						else
						{
							if (Status == HTTPOK)
							{						
								Status = ParseHeaderField (Line);
							}
						}

						nChar = 0;
					}
				}
				else
				{
					// accumulate option line
					if (nChar < sizeof Line-1)
					{
						Line[nChar++] = chChar;
						Line[nChar] = '\0';
					}
					else
					{
						Status = HTTPRequestEntityTooLarge;
					}
				}
			}
			else if (nState == 1)
			{
				m_RequestFormData[nChar++] = chChar;
				m_RequestFormData[nChar] = '\0';

				if (nChar >= m_nRequestContentLength)
				{
					nState = 3;
				}
			}
			else if (nState == 2)
			{
				m_pMultipartBuffer[nChar++] = chChar;

				if (nChar >= m_nMultipartContentLength)
				{
					m_pMultipartPointer = m_pMultipartBuffer;

					nState = 3;
				}
			}
		}
	}

	if (nResult < 0)
	{
		CLogger::Get ()->Write (FromHTTPDaemon, LogError, "Receive failed");

		return HTTPUnknownError;
	}
	
	if (Status != HTTPOK)
	{
		return Status;
	}
	
	if (nLine == 0)
	{
		return HTTPUnknownError;
	}

	// check for parameters
	const char *pParams = strchr (m_RequestURI, '?');
	if (pParams != 0)
	{
		strncpy (m_RequestPath, m_RequestURI, pParams-m_RequestURI);
		m_RequestPath[pParams-m_RequestURI] = '\0';

		strcpy (m_RequestParams, pParams+1);
	}
	else
	{
		strcpy (m_RequestPath, m_RequestURI);
	}

	return HTTPOK;
}

THTTPStatus CHTTPDaemon::ParseMethod (char *pLine)
{
	// "METHOD uri HTTP/1.1" expected

	char *pToken;
	char *pSavePtr;

	assert (pLine != 0);
	if ((pToken = strtok_r (pLine, " ", &pSavePtr)) == 0)
	{
		return HTTPMethodNotImplemented;
	}

	if (strcmp (pToken, "GET") == 0)
	{
		m_RequestMethod = HTTPRequestMethodGet;
	}
	else if (strcmp (pToken, "HEAD") == 0)
	{
		m_RequestMethod = HTTPRequestMethodHead;
	}
	else if (strcmp (pToken, "POST") == 0)
	{
		m_RequestMethod = HTTPRequestMethodPost;
	}
	else
	{
		return HTTPMethodNotImplemented;
	}

	if ((pToken = strtok_r (0, " ", &pSavePtr)) == 0)
	{
		return HTTPBadRequest;
	}

	if (strlen (pToken) > sizeof m_RequestURI-1)
	{
		return HTTPRequestURITooLong;
	}

	strcpy (m_RequestURI, pToken);

	if (   (pToken = strtok_r (0, "/", &pSavePtr)) == 0
	    || strcmp (pToken, "HTTP") != 0)
	{
		return HTTPBadRequest;
	}

	if ((pToken = strtok_r (0, " \n", &pSavePtr)) == 0)
	{
		return HTTPBadRequest;
	}

	if (strcmp (pToken, "1.1") != 0)
	{
		return HTTPVersionNotSupported;
	}

	return HTTPOK;
}

THTTPStatus CHTTPDaemon::ParseHeaderField (char *pLine)
{
	char *pToken;
	char *pSavePtr;

	assert (pLine != 0);
	if ((pToken = strtok_r (pLine, ":", &pSavePtr)) == 0)
	{
		return HTTPBadRequest;
	}

	if (strcmp (pToken, "Content-Type") == 0)
	{
		if ((pToken = strtok_r (0, " ;", &pSavePtr)) == 0)
		{
			return HTTPBadRequest;
		}

		if (strcmp (pToken, "application/x-www-form-urlencoded") == 0)
		{
			m_bRequestFormDataAvailable = TRUE;
		}
		else if (strcmp (pToken, "multipart/form-data") == 0)
		{
			if (   (pToken = strtok_r (0, " =", &pSavePtr)) == 0
			    || strcmp (pToken, "boundary") != 0
			    || (pToken = strtok_r (0, ";", &pSavePtr)) == 0
			    || strlen (pToken) > HTTP_MAX_MULTIPART_BOUNDARY)
			{
				return HTTPBadRequest;
			}

			m_bMultipartFormDataAvailable = TRUE;

			strcpy (m_MultipartBoundary, pToken);
		}
	}
	else if (strcmp (pToken, "Content-Length") == 0)
	{
		if ((pToken = strtok_r (0, " ", &pSavePtr)) == 0)
		{
			return HTTPBadRequest;
		}

		unsigned nAccu = 0;
		while (*pToken != '\0')
		{
			unsigned nDigit = *pToken++ - '0';
			if (nDigit > 9)
			{
				return HTTPBadRequest;
			}

			nAccu *= 10;
			nAccu += nDigit;

			if (nAccu > 1000000000U)	// prevent wrapping
			{
				return HTTPRequestEntityTooLarge;
			}
		}

		m_nRequestContentLength = nAccu;
	}

	return HTTPOK;
}

boolean CHTTPDaemon::GetMultipartFormPart (const char **ppHeader,
					   const u8 **ppData, unsigned *pLength)
{
	if (   !m_bMultipartFormDataAvailable
	    || m_pMultipartPointer == 0)
	{
		return FALSE;
	}

	assert (ppHeader != 0);
	*ppHeader = m_pMultipartPointer;

	// find end of part header
	char *p = (char *) Search (m_pMultipartPointer, m_nMultipartContentLength, "\r\n\r\n", 4);
	if (p == 0)
	{
		return FALSE;
	}

	*p = '\0';	// terminate header

	p += 4;
	assert (ppData != 0);
	*ppData = (u8 *) p;

	m_nMultipartContentLength -= p - m_pMultipartPointer;
	m_pMultipartPointer = p;

	// find end of part data
	size_t nBoundaryLen = strlen (m_MultipartBoundary);
	assert (nBoundaryLen > 0);

	char *q = (char *) Search (p, m_nMultipartContentLength, m_MultipartBoundary, nBoundaryLen);
	if (q == 0)
	{
		return FALSE;
	}

	if (q - p < 4)
	{
		return FALSE;
	}
	q -= 4;

	if (memcmp (q, "\r\n--", 4) != 0)
	{
		return FALSE;
	}

	unsigned nLength = q - p;

	assert (pLength != 0);
	*pLength = nLength;

	m_nMultipartContentLength -= nLength;
	m_pMultipartPointer += nLength;

	// advance m_pMultipartPointer to next part (if any)
	nLength = nBoundaryLen + 4;

	assert (m_nMultipartContentLength >= nLength);
	m_nMultipartContentLength -= nLength;
	m_pMultipartPointer += nLength;

	if (m_nMultipartContentLength >= nBoundaryLen)		// another part follows?
	{
		m_nMultipartContentLength -= 2;
		m_pMultipartPointer += 2;
	}
	else
	{
		m_pMultipartPointer = 0;			// no: next call will fail
	}

	return TRUE;
}

// TODO: optimize
void *CHTTPDaemon::Search (const void *pBuffer, unsigned nBufLen,
			   const void *pNeedle, unsigned nNeedleLen)
{
	if (nNeedleLen == 0)
	{
		return (void *) pBuffer;
	}

	const u8 *puchBuffer = (const u8 *) pBuffer;
	const u8 *puchNeedle = (const u8 *) pNeedle;
	while (nNeedleLen <= nBufLen)
	{
		if (!memcmp (puchBuffer, puchNeedle, nNeedleLen))
		{
			return (void *) puchBuffer;
		}

		puchBuffer++;
		nBufLen--;
	}

	return 0;
}
