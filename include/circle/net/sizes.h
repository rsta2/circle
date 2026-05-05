//
// sizes.h
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
#ifndef _circle_net_sizes_h
#define _circle_net_sizes_h

// Ethernet (without CRC)
#define ETH_MIN_LEN		60		// minimum length on wire
#define ETH_MAX_LEN		1514		// maximum length on wire
#define ETH_HEADER_LEN		(6+6+2)

// IP
#define IP_HEADER_LEN		(5*4)		// no option
#define IP_RA_HEADER_LEN	(5*4+4)		// with Router Alert option

// UDP
#define UDP_HEADER_LEN		(2*4)		// no option

// TCP
#define TCP_HEADER_LEN		(5*4)		// no option
#define TCP_MSS_HEADER_LEN	(5*4+4)		// with MSS option

#endif
