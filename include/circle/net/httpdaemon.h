//
// httpdaemon.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_net_httpdaemon_h
#define _circle_net_httpdaemon_h

#include <circle/sched/task.h>
#include <circle/net/netsubsystem.h>
#include <circle/net/socket.h>
#include <circle/types.h>

#define HTTP_PORT		80

#define HTTP_MAX_REQUEST_LINE	2048
#define HTTP_MAX_URI		HTTP_MAX_REQUEST_LINE
#define HTTP_MAX_PATH		256
#define HTTP_MAX_PARAMS		(HTTP_MAX_URI-HTTP_MAX_PATH-1)
#define HTTP_MAX_FORM_DATA	2048

enum THTTPRequestMethod
{
	HTTPRequestMethodGet,
	HTTPRequestMethodHead,
	HTTPRequestMethodPost,
	HTTPRequestMethodUnknown
};

enum THTTPStatus
{
	HTTPOK			  = 200,
	HTTPBadRequest		  = 400,
	HTTPNotFound		  = 404,
	HTTPRequestEntityTooLarge = 413,
	HTTPRequestURITooLong	  = 414,
	HTTPInternalServerError	  = 500,
	HTTPMethodNotImplemented  = 501,
	HTTPVersionNotSupported	  = 505,
	HTTPUnknownError	  = 520
};

class CHTTPDaemon : public CTask
{
public:
	CHTTPDaemon (CNetSubSystem *pNetSubSystem,
		     CSocket	   *pSocket	    = 0,	// is 0 for 1st created instance (listener)
		     unsigned	    nMaxContentSize = 0,	// buffer size for worker
		     u16	    nPort	    = HTTP_PORT);
	~CHTTPDaemon (void);

	void Run (void);

	// creates an instance of your derived webserver class
	virtual CHTTPDaemon *CreateWorker (CNetSubSystem *pNetSubSystem, CSocket *pSocket) = 0;

	// define this to provide your content
	virtual THTTPStatus GetContent (const char  *pPath,	// path of the file to be sent
				        const char  *pParams,	// parameters to GET ("" for none)
					const char  *pFormData, // form data from POST ("" for none)
				        u8	    *pBuffer,	// copy your content here
				        unsigned    *pLength,	// in: buffer size, out: content length
				        const char **ppContentType) = 0; // set this if not "text/html"

private:
	void Listener (void);			// accepts incoming connections and creates worker task
	void Worker (void);			// processes a connection

	THTTPStatus ParseRequest (void);
	THTTPStatus ParseMethod (char *pLine);
	THTTPStatus ParseHeaderField (char *pLine);

private:
	CNetSubSystem *m_pNetSubSystem;
	CSocket	      *m_pSocket;
	unsigned       m_nMaxContentSize;
	u16	       m_nPort;
	
	u8 *m_pContentBuffer;

	// from request
	THTTPRequestMethod m_RequestMethod;

	char m_RequestURI[HTTP_MAX_URI+1];		// the URI without host
	char m_RequestPath[HTTP_MAX_PATH+1];		// the path without parameters
	char m_RequestParams[HTTP_MAX_PARAMS+1];	// the parameters from URI

	boolean m_bRequestFormDataAvailable;		// form data is available
	unsigned m_nRequestContentLength;		// length of form data from POST request
	char m_RequestFormData[HTTP_MAX_FORM_DATA+1];	// form data from POST request

	static unsigned s_nInstanceCount;
};

#endif
