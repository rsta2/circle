//
/// \file debug.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2025  R. Stange <rsta2@gmx.net>
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
#ifndef _circle_debug_h
#define _circle_debug_h

#include <circle/types.h>

#if AARCH == 32
#define Breakpoint(id)		asm volatile ("bkpt %0" :: "i" (id))
#else
#define Breakpoint(id)		asm volatile ("brk %0" :: "i" (id))
#endif

//#define DEBUG_CLICK

#define DEBUG_HEXDUMP_HEADER	1	///< Show address and size header
#define DEBUG_HEXDUMP_ADDRESS	2	///< Show address instead of offset
#define DEBUG_HEXDUMP_ASCII	4	///< Include ASCII dump
void DebugHexDump (const void *pStart, unsigned nBytes, const char *pSource = 0,
		   unsigned nFlags = DEBUG_HEXDUMP_HEADER);

#define debug_hexdump		DebugHexDump	///< Deprecated

void DebugStackTrace (const uintptr *pStackPtr, const char *pSource = 0);

#define debug_stacktrace	DebugStackTrace	///< Deprecated

#ifdef DEBUG_CLICK

/// Left and right may be swapped
#define DEBUG_CLICK_LEFT	1
#define DEBUG_CLICK_RIGHT	2
#define DEBUG_CLICK_ALL		(DEBUG_CLICK_LEFT | DEBUG_CLICK_RIGHT)
void DebugClick (unsigned nMask = DEBUG_CLICK_ALL);

#define debug_click		DebugClick	///< Deprecated

#endif

#endif
