//
// httpclient.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_net_httpclient_h
#define _circle_net_httpclient_h

#include <circle/net/netsubsystem.h>
#include <circle/net/ipaddress.h>
#include <circle/net/http.h>
#include <circle/net/socket.h>
#include <circle/string.h>
#include <circle/types.h>

class CHTTPClient
{
public:
	CHTTPClient (CNetSubSystem *pNetSubSystem,
		     CIPAddress	   &rServerIP,
		     u16	    nServerPort = HTTP_PORT,
		     const char	   *pServerName = 0);		// required for virtual servers
	~CHTTPClient (void);

	THTTPStatus Get (const char *pPath,			// "/file[?name=value[&name=value...]]"
			 u8	    *pBuffer,			// content will be returned here
			 unsigned   *pLength);			// in: buffer size, out: content length

	THTTPStatus Post (const char *pPath,			// "/file[?name=value[&name=value...]]"
			  u8	     *pBuffer,			// content will be returned here
			  unsigned   *pLength,			// in: buffer size, out: content length
			  const char *pFormData);		// "name=value[&name=value...]"

private:
	THTTPStatus Request (THTTPRequestMethod	 Method,
			     const char		*pPath,		// may include URL parameters
			     u8			*pBuffer,	// content will be returned here
			     unsigned		*pLength,	// in: buffer size, out: content length
			     const char		*pFormData = 0);// form data for POST or 0

private:
	CNetSubSystem *m_pNetSubSystem;
	CIPAddress     m_ServerIP;
	u16	       m_ServerPort;
	CString	       m_ServerName;

	CSocket	      *m_pSocket;
};

#endif
