//
// httpbootserver.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
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
#include "httpbootserver.h"
#include <circle/chainboot.h>
#include <circle/logger.h>
#include <circle/machineinfo.h>
#include <circle/string.h>
#include <circle/util.h>
#include <circle/version.h>
#include <assert.h>

#define MAX_CONTENT_SIZE	4000

// our content
static const char s_Index[] =
#include "index.h"
;

static const u8 s_Style[] =
#include "style.h"
;

static const u8 s_Favicon[] =
{
#include "favicon.h"
};

static const char FromHTTPBootServer[] = "httpboot";

CHTTPBootServer::CHTTPBootServer (CNetSubSystem *pNetSubSystem, u16 nPort,
				  unsigned nMaxMultipartSize, CSocket *pSocket)
:	CHTTPDaemon (pNetSubSystem, pSocket, MAX_CONTENT_SIZE, nPort, nMaxMultipartSize),
	m_nPort (nPort),
	m_nMaxMultipartSize (nMaxMultipartSize)
{
}

CHTTPBootServer::~CHTTPBootServer (void)
{
}

CHTTPDaemon *CHTTPBootServer::CreateWorker (CNetSubSystem *pNetSubSystem, CSocket *pSocket)
{
	return new CHTTPBootServer (pNetSubSystem, m_nPort, m_nMaxMultipartSize, pSocket);
}

THTTPStatus CHTTPBootServer::GetContent (const char  *pPath,
					 const char  *pParams,
					 const char  *pFormData,
					 u8	     *pBuffer,
					 unsigned    *pLength,
					 const char **ppContentType)
{
	assert (pPath != 0);
	assert (ppContentType != 0);

	CString String;
	const u8 *pContent = 0;
	unsigned nLength = 0;

	if (   strcmp (pPath, "/") == 0
	    || strcmp (pPath, "/index.html") == 0)
	{
		const char *pMsg = 0;

		const char *pPartHeader;
		const u8 *pPartData;
		unsigned nPartLength;
		if (GetMultipartFormPart (&pPartHeader, &pPartData, &nPartLength))
		{
			assert (pPartHeader != 0);
			if (   strstr (pPartHeader, "name=\"kernelimg\"") != 0
			    && strstr (pPartHeader, "filename=\"kernel") != 0
			    && strstr (pPartHeader, ".img\"") != 0
			    && nPartLength > 0)
			{
				u8 *pKernelImage = new u8[nPartLength];
				if (pKernelImage != 0)
				{
					assert (pPartData != 0);
					memcpy (pKernelImage, pPartData, nPartLength);

					EnableChainBoot (pKernelImage, nPartLength);

					pMsg = "Now booting...";
				}
				else
				{
					pMsg = "Out of memory";
				}
			}
			else
			{
				pMsg = "Invalid request";
			}
		}
		else
		{
			pMsg = "Select the kernel image file to be loaded "
			       "and press the boot button!";
		}

		assert (pMsg != 0);
		String.Format (s_Index, pMsg, CIRCLE_VERSION_STRING,
			       CMachineInfo::Get ()->GetMachineName ());

		pContent = (const u8 *) (const char *) String;
		nLength = String.GetLength ();
		*ppContentType = "text/html; charset=iso-8859-1";
	}
	else if (strcmp (pPath, "/style.css") == 0)
	{
		pContent = s_Style;
		nLength = sizeof s_Style-1;
		*ppContentType = "text/css";
	}
	else if (strcmp (pPath, "/favicon.ico") == 0)
	{
		pContent = s_Favicon;
		nLength = sizeof s_Favicon;
		*ppContentType = "image/x-icon";
	}
	else
	{
		return HTTPNotFound;
	}

	assert (pLength != 0);
	if (*pLength < nLength)
	{
		CLogger::Get ()->Write (FromHTTPBootServer, LogError,
					"Increase MAX_CONTENT_SIZE to at least %u", nLength);

		return HTTPInternalServerError;
	}

	assert (pBuffer != 0);
	assert (pContent != 0);
	assert (nLength > 0);
	memcpy (pBuffer, pContent, nLength);

	*pLength = nLength;

	return HTTPOK;
}
