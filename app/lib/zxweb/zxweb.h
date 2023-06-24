//
// zxweb.h
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
#ifndef _zxweb_h
#define _zxweb_h

#include <circle/net/httpdaemon.h>
#include <circle/actled.h>

// Move outside of the web server - it should be transparent
// enum TZxWebAction
// {
// 	// Tape actions
// 	TZxWebActionTapeLoad = 100,
// 	TZxWebActionTapePlayPause = 101,
// 	TZxWebActionTapeRewind = 102,
// 	TZxWebActionTapeFastForward = 103,
// 	TZxWebActionTapeStop = 104,
// };

typedef void TZxWebActionCallback (const char *pAction, const void *pData, void *pParam);


class CZxWeb : public CHTTPDaemon
{
public:
	CZxWeb (CNetSubSystem *pNetSubSystem, CSocket *pSocket = 0);	// is 0 for 1st created instance (listener)
	~CZxWeb (void);

	// creates an instance of our derived webserver class
	CHTTPDaemon *CreateWorker (CNetSubSystem *pNetSubSystem, CSocket *pSocket);

	// provides our content
	THTTPStatus GetContent (const char  *pPath,		// path of the file to be sent
							const char  *pParams,		// parameters to GET ("" for none)
							const char  *pFormData, 	// form data from POST ("" for none)
							u8	    *pBuffer,			// copy your content here
							unsigned    *pLength,		// in: buffer size, out: content length
							const char **ppContentType	// set this if not "text/html"
	);

	void SetWebActionRoutine(TZxWebActionCallback *pRoutine, void *pParam);
// private:
	//

private:
	TZxWebActionCallback *m_pWebActionRoutine;
	void *m_pWebActionParam;
};

#endif // _zxweb_h
