//
// exception.h
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
#ifndef _circle_exception_h
#define _circle_exception_h

#if AARCH == 32

#define EXCEPTION_DIVISION_BY_ZERO		0
#define EXCEPTION_UNDEFINED_INSTRUCTION		1
#define EXCEPTION_PREFETCH_ABORT		2
#define EXCEPTION_DATA_ABORT			3
#define EXCEPTION_UNKNOWN			4

#else

#define EXCEPTION_UNEXPECTED			0
#define EXCEPTION_SYNCHRONOUS			1
#define EXCEPTION_SYSTEM_ERROR			2

#endif

#endif
