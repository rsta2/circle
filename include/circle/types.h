//
// types.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2018  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_types_h
#define _circle_types_h

#include <assert.h>

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;

typedef signed char		s8;
typedef signed short		s16;
typedef signed int		s32;

#if AARCH == 32
typedef unsigned long long	u64;
typedef signed long long	s64;

typedef int			intptr;
typedef unsigned int		uintptr;

typedef unsigned int		size_t;
typedef int			ssize_t;
#else
typedef unsigned long		u64;
typedef signed long		s64;

typedef long			intptr;
typedef unsigned long		uintptr;

typedef unsigned long		size_t;
typedef long			ssize_t;
#endif

#ifdef __cplusplus
typedef bool		boolean;
#define FALSE		false
#define TRUE		true
#else
typedef char		boolean;
#define FALSE		0
#define TRUE		1
#endif
ASSERT_STATIC (sizeof (boolean) == 1);

#endif
