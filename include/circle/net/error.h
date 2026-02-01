//
/// \file error.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2025  R. Stange <rsta2@gmx.net>
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
#ifndef _circle_net_error_h
#define _circle_net_error_h

// These codes will be returned as negative values from socket functions.

							// Linux errno equivalent:
#define NET_ERROR_UNKNOWN			1

#define NET_ERROR_IO				11	// EIO
#define NET_ERROR_WOULD_BLOCK			12	// EWOULDBLOCK
#define NET_ERROR_PERMISSION_DENIED		13	// EACCES
#define NET_ERROR_INVALID_VALUE			14	// EINVAL

#define NET_ERROR_PROTOCOL_ERROR		51	// EPROTO
#define NET_ERROR_PROTOCOL_NOT_SUPPORTED	52	// EPROTONOSUPPORT
#define NET_ERROR_OPERATION_NOT_SUPPORTED	53	// EOPNOTSUPP
#define NET_ERROR_CONNECTION_RESET		54	// ECONNRESET
#define NET_ERROR_IS_CONNECTED			55	// EISCONN
#define NET_ERROR_NOT_CONNECTED			56	// ENOTCONN
#define NET_ERROR_CONNECTION_TIMED_OUT		57	// ETIMEDOUT
#define NET_ERROR_CONNECTION_REFUSED		58	// ECONNREFUSED
#define NET_ERROR_DESTINATION_UNREACHABLE	59	// EHOSTUNREACH

#endif
