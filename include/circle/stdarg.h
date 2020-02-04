//
// stdarg.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_stdarg_h
#define _circle_stdarg_h

// prevent warning, if <stdarg.h> from toolchain is included too
#ifndef _STDARG_H

typedef __builtin_va_list va_list;

#define va_start(arg, last)	__builtin_va_start (arg, last)
#define va_end(arg)		__builtin_va_end (arg)
#define va_arg(arg, type)	__builtin_va_arg (arg, type)

#endif

#endif
