//
// http.h
//
// Definitions common to HTTP client and server
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
#ifndef _circle_net_http_h
#define _circle_net_http_h

#define HTTP_PORT		80

#define HTTP_MAX_REQUEST_LINE	2048
#define HTTP_MAX_URI		HTTP_MAX_REQUEST_LINE
#define HTTP_MAX_PATH		256
#define HTTP_MAX_PARAMS		(HTTP_MAX_URI-HTTP_MAX_PATH-1)
#define HTTP_MAX_FORM_DATA	2048
#define HTTP_MAX_MULTIPART_BOUNDARY 100

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
	HTTPRequestTimeout	  = 408,
	HTTPRequestEntityTooLarge = 413,
	HTTPRequestURITooLong	  = 414,
	HTTPInternalServerError	  = 500,
	HTTPMethodNotImplemented  = 501,
	HTTPVersionNotSupported	  = 505,
	HTTPUnknownError	  = 520,

	// self defined codes (for the client only)
	HTTPConnectionReset	  = 550,
	HTTPInvalidResponseCode	  = 551,
	HTTPInvalidChunkHeader	  = 552,
	HTTPContentBufferTooSmall = 553
};

#endif
