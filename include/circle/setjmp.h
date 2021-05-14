//
/// \file setjmp.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020-2021  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_setjmp_h
#define _circle_setjmp_h

#if STDLIB_SUPPORT >= 3
	#include <setjmp.h>
#else

#include <circle/macros.h>
#include <circle/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if AARCH == 32
typedef u32	jmp_buf[11];
#else
typedef u64	jmp_buf[14];
#endif

int setjmp (jmp_buf env);

void longjmp (jmp_buf env, int val) NORETURN;

#ifdef __cplusplus
}
#endif

#endif

#endif
