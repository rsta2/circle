//
// zxweb.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015  R. Stange <rsta2@o2online.de>
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
#include "zxweb.h"
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/util.h>
#include <assert.h>

#define MAX_CONTENT_SIZE	4000
#define PATH_ACTION 		"/action/"
#define PATH_ACTION_LEN		sizeof(PATH_ACTION) - 1

#define CONTENT_TYPE_JSON	"application/json; charset=iso-8859-1"

#define JSON_STATUS_OK	    "{\"status\":\"OK\"}"

LOGMODULE ("ZxWeb");


// our content
static const char s_Index[] =
#include "index.h"
;

static const u8 s_Style[] =
#include "style.h"
;

static const u8 s_LEDOff[] =
{
#include "ledoff.h"
};

static const u8 s_LEDOn[] =
{
#include "ledon.h"
};

static const u8 s_Favicon[] =
{
#include "favicon.h"
};

static const char FromWebServer[] = "zxweb";

CZxWeb::CZxWeb (CNetSubSystem *pNetSubSystem, CSocket *pSocket):
	CHTTPDaemon (pNetSubSystem, pSocket, MAX_CONTENT_SIZE),
	m_pWebActionRoutine (0),
	m_pWebActionParam (0)
{
	//
}

CZxWeb::~CZxWeb (void)
{
	//
}

CHTTPDaemon *CZxWeb::CreateWorker (CNetSubSystem *pNetSubSystem, CSocket *pSocket)
{
	return new CZxWeb (pNetSubSystem, pSocket);
}



THTTPStatus CZxWeb::GetContent (const char  *pPath,
				    const char  *pParams,
				    const char  *pFormData,
				    u8	        *pBuffer,
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
		assert (pFormData != 0);
		if (strcmp (pFormData, "actled=1") == 0)
		{
			// m_pActLED->On ();
			String.Format (s_Index, "", "checked", "ledon.png");	// must match index.html
		}
		else
		{
			// m_pActLED->Off ();
			String.Format (s_Index, "checked", "", "ledoff.png");
		}

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
	else if (strcmp (pPath, "/ledoff.png") == 0)
	{
		pContent = s_LEDOff;
		nLength = sizeof s_LEDOff;
		*ppContentType = "image/png";
	}
	else if (strcmp (pPath, "/ledon.png") == 0)
	{
		pContent = s_LEDOn;
		nLength = sizeof s_LEDOn;
		*ppContentType = "image/png";
	}
	else if (strcmp (pPath, "/favicon.ico") == 0)
	{
		pContent = s_Favicon;
		nLength = sizeof s_Favicon;
		*ppContentType = "image/x-icon";
	}
	else if (strlen(pPath) > PATH_ACTION_LEN && strncmp (pPath, PATH_ACTION, PATH_ACTION_LEN) == 0)
	{
		const char *pAction = pPath + PATH_ACTION_LEN;
		// TODO get pData;

		// TODO queue action routine (don't callback in this context!)

		// Return OK
		String.Format(JSON_STATUS_OK);

		pContent = (const u8 *) (const char *) String;
		nLength = String.GetLength ();
		*ppContentType = CONTENT_TYPE_JSON;
	}
	else
	{
		return HTTPNotFound;
	}

	assert (pLength != 0);
	if (*pLength < nLength)
	{
		LOGERR("Increase MAX_CONTENT_SIZE to at least %u", nLength);

		return HTTPInternalServerError;
	}

	assert (pBuffer != 0);
	assert (pContent != 0);
	assert (nLength > 0);
	memcpy (pBuffer, pContent, nLength);

	*pLength = nLength;

	return HTTPOK;
}

void CZxWeb::SetWebActionRoutine (TZxWebActionCallback *pRoutine, void *pParam)
{
	m_pWebActionRoutine = pRoutine;
	m_pWebActionParam = pParam;
}