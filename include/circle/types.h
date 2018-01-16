//
// types.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2017  R. Stange <rsta2@o2online.de>
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
typedef unsigned long long	u64;

typedef signed char		s8;
typedef signed short		s16;
typedef signed int		s32;
typedef signed long long	s64;

typedef int			intptr;
typedef unsigned int		uintptr;

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

typedef unsigned int	size_t;
typedef int		ssize_t;

#endif
