//
// httpclient.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2018  R. Stange <rsta2@o2online.de>
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
#include <circle/net/httpclient.h>
#include <circle/netdevice.h>
#include <circle/util.h>
#include <circle/net/in.h>
#include <assert.h>

#define CLIENT_VERSION	"0.02"
#define USER_AGENT	"CHTTPClient/" CLIENT_VERSION " (Circle)"

CHTTPClient::CHTTPClient (CNetSubSystem	*pNetSubSystem,
			  CIPAddress	&rServerIP,
			  u16	    	 nServerPort,
			  const char	*pServerName)
:	m_pNetSubSystem (pNetSubSystem),
	m_ServerIP (rServerIP),
	m_ServerPort (nServerPort),
	m_ServerName (pServerName),
	m_pSocket (0)
{
}

CHTTPClient::~CHTTPClient (void)
{
	delete m_pSocket;
	m_pSocket = 0;

	m_pNetSubSystem = 0;
}

THTTPStatus CHTTPClient::Get (const char *pPath, u8 *pBuffer, unsigned *pLength)
{
	return Request (HTTPRequestMethodGet, pPath, pBuffer, pLength);
}

THTTPStatus CHTTPClient::Post (const char *pPath, u8 *pBuffer, unsigned *pLength, const char *pFormData)
{
	assert (pFormData != 0);
	return Request (HTTPRequestMethodPost, pPath, pBuffer, pLength, pFormData);
}

THTTPStatus CHTTPClient::Request (THTTPRequestMethod  Method,
				  const char	     *pPath,
				  u8		     *pBuffer,
				  unsigned	     *pLength,
				  const char	     *pFormData)
{
	// connect to server
	assert (m_pNetSubSystem != 0);
	assert (m_pSocket == 0);
	m_pSocket = new CSocket (m_pNetSubSystem, IPPROTO_TCP);
	assert (m_pSocket != 0);
	if (m_pSocket->Connect (m_ServerIP, m_ServerPort) < 0)
	{
		delete m_pSocket;
		m_pSocket = 0;

		return HTTPRequestTimeout;
	}

	// send HTTP request
	const char *pMethod = 0;
	switch (Method)
	{
	case HTTPRequestMethodGet:	pMethod = "GET";	break;
	case HTTPRequestMethodPost:	pMethod = "POST";	break;

	default:
		assert (0);
		break;
	}

	CString Request;
	assert (pMethod != 0);
	assert (pPath != 0);
	Request.Format ("%s %s HTTP/1.1\r\n", pMethod, pPath);

	if (m_ServerName.GetLength () > 0)
	{
		Request.Append ("Host: ");
		Request.Append (m_ServerName);
		Request.Append ("\r\n");
	}

	Request.Append ("User-Agent: " USER_AGENT "\r\n");
	Request.Append ("Connection: close\r\n");

	if (pFormData != 0)
	{
		Request.Append ("Content-Type: application/x-www-form-urlencoded\r\n");

		CString ContentLength;
		ContentLength.Format ("Content-Length: %u\r\n", (unsigned) strlen (pFormData));
		Request.Append (ContentLength);
	}

	Request.Append ("\r\n");

	if (pFormData != 0)
	{
		Request.Append (pFormData);
	}

	if (m_pSocket->Send (Request, Request.GetLength (), 0) < 0)
	{
		delete m_pSocket;
		m_pSocket = 0;

		return HTTPConnectionReset;
	}

	// receive HTTP response and parse it
	unsigned nState = 0;
	unsigned nLine = 0;
	unsigned nChar = 0;
	unsigned nLength = 0;
	boolean bChunked = FALSE;
	unsigned long ulBytes = 0;

	char Buffer[FRAME_BUFFER_SIZE];
	char Line[HTTP_MAX_REQUEST_LINE];
	int nResult;
	char *pSavePtr;

	while (   nState < 5
	       && (nResult = m_pSocket->Receive (Buffer, sizeof Buffer, 0)) > 0)
	{
		for (int i = 0; i < nResult; i++)
		{
			u8 chChar = Buffer[i];

			switch (nState)
			{
			case 0:				// response header
				if (chChar == '\r')
				{
					continue;
				}

				if (chChar == '\n')		// end of line
				{
					if (nChar == 0)		// empty line is end of header
					{
						nState = bChunked ? 2 : 1;
						nChar = 0;
					}
					else
					{
						if (nLine++ == 0)	// first line?
						{
							// "HTTP/1.x 200 OK" expected
							char *pToken;
							if (   (pToken = strtok_r (Line, "/", &pSavePtr)) == 0
							    || strcmp (pToken, "HTTP") != 0
							    || (pToken = strtok_r (0, " ", &pSavePtr)) == 0
							    || (pToken = strtok_r (0, " ", &pSavePtr)) == 0
							    || strcmp (pToken, "200") != 0)
							{
								char *pEnd;
								unsigned long ulStatus;
								ulStatus = strtoul (pToken, &pEnd, 10);
								if (   pEnd != 0
								    && *pEnd != '\0')
								{
									ulStatus = HTTPInvalidResponseCode;
								}

								delete m_pSocket;
								m_pSocket = 0;

								return (THTTPStatus) ulStatus;
							}
						}
						else
						{
							// check for transfer encoding option
							char *pToken = strtok_r (Line, ": ", &pSavePtr);
							if (   pToken != 0
							    && strcasecmp (pToken, "Transfer-Encoding") == 0)
							{
								pToken = strtok_r (0, " ", &pSavePtr);
								if (pToken != 0
								    && strcasecmp (pToken, "chunked") == 0)
								{
									bChunked = TRUE;
								}
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
				}
				break;

			case 1:				// non-chunked data: simply copy it
				assert (pLength != 0);
				if (nLength >= *pLength)
				{
					delete m_pSocket;
					m_pSocket = 0;

					return HTTPContentBufferTooSmall;
				}

				*pBuffer++ = (u8) chChar;
				nLength++;
				break;

			case 2:				// chunk header
				if (chChar == '\r')
				{
					continue;
				}

				if (chChar == '\n')	// end of header?
				{
					char *pEnd;
					ulBytes = strtoul (Line, &pEnd, 16);	// convert chunk length
					if (   pEnd != 0
					    && *pEnd != '\0')
					{
						delete m_pSocket;
						m_pSocket = 0;

						return HTTPInvalidChunkHeader;
					}

					nState = ulBytes != 0 ? 3 : 5;	// length 0 is end of file
				}
				else
				{
					// accumulate chunk header line
					if (nChar < sizeof Line-1)
					{
						Line[nChar++] = chChar;
						Line[nChar] = '\0';
					}
				}
				break;

			case 3:				// chunk data: copy ulBytes
				assert (pLength != 0);
				if (nLength >= *pLength)
				{
					delete m_pSocket;
					m_pSocket = 0;

					return HTTPContentBufferTooSmall;
				}

				*pBuffer++ = (u8) chChar;
				nLength++;

				if (--ulBytes == 0)
				{
					nState = 4;
				}
				break;

			case 4:				// chunk trailer
				if (chChar == '\r')
				{
					continue;
				}

				if (chChar != '\n')	// newline expected
				{
					delete m_pSocket;
					m_pSocket = 0;

					return HTTPInvalidChunkHeader;
				}

				nChar = 0;
				nState = 2;
				break;
			}
		}
	}

	// close everything and exit
	if (   nState < 5
	    && nState != 1)
	{
		delete m_pSocket;
		m_pSocket = 0;

		return HTTPConnectionReset;
	}

	delete m_pSocket;
	m_pSocket = 0;

	assert (pLength != 0);
	assert (nLength <= *pLength);
	*pLength = nLength;

	return HTTPOK;
}
