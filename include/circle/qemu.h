//
// qemu.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_qemu_h
#define _circle_qemu_h

#include <circle/types.h>

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////
//
// ARM semihosting (aka "Angel") interface
//
// See: https://static.docs.arm.com/100863/0200/semihosting.pdf
//
///////////////////////////////////////////////////////////////////////

#define SEMIHOSTING_SYS_OPEN		0x01
#define SEMIHOSTING_SYS_CLOSE		0x02
#define SEMIHOSTING_SYS_WRITEC		0x03
#define SEMIHOSTING_SYS_WRITE0		0x04
#define SEMIHOSTING_SYS_WRITE		0x05
#define SEMIHOSTING_SYS_READ		0x06
#define SEMIHOSTING_SYS_READC		0x07
#define SEMIHOSTING_SYS_ISTTY		0x09
#define SEMIHOSTING_SYS_SEEK		0x0A
#define SEMIHOSTING_SYS_FLEN		0x0C
#define SEMIHOSTING_SYS_TMPNAM		0x0D
#define SEMIHOSTING_SYS_REMOVE		0x0E
#define SEMIHOSTING_SYS_RENAME		0x0F
#define SEMIHOSTING_SYS_CLOCK		0x10
#define SEMIHOSTING_SYS_TIME		0x11
#define SEMIHOSTING_SYS_SYSTEM		0x12
#define SEMIHOSTING_SYS_ERRNO		0x13
#define SEMIHOSTING_SYS_GET_CMDLINE	0x15
#define SEMIHOSTING_SYS_HEAPINFO	0x16
#define SEMIHOSTING_SYS_EXIT		0x18
#define SEMIHOSTING_SYS_EXIT_EXTENDED	0x20

#if AARCH == 32
	typedef u32 TSemihostingValue;
#else
	typedef u64 TSemihostingValue;
#endif

TSemihostingValue CallSemihostingSingle (u32 nOperation, TSemihostingValue nParam = 0);

TSemihostingValue CallSemihosting (u32 nOperation,
				   TSemihostingValue nParam1 = 0, TSemihostingValue nParam2 = 0,
				   TSemihostingValue nParam3 = 0, TSemihostingValue nParam4 = 0);


#define SEMIHOSTING_FEATURE(bit)	BIT (bit)	// only byte 0 features are supported

#define SEMIHOSTING_EXT_EXIT_EXTENDED	SEMIHOSTING_FEATURE (0)
#define SEMIHOSTING_EXT_STDOUT_STDERR	SEMIHOSTING_FEATURE (1)

boolean SemihostingFeatureSupported (u32 nFeature);


// SEMIHOSTING_SYS_OPEN mode parameter
#define SEMIHOSTING_OPEN_READ		0
#define SEMIHOSTING_OPEN_READ_BIN	1
#define SEMIHOSTING_OPEN_READ_PLUS	2
#define SEMIHOSTING_OPEN_READ_PLUS_BIN	3
#define SEMIHOSTING_OPEN_WRITE		4
#define SEMIHOSTING_OPEN_WRITE_BIN	5
#define SEMIHOSTING_OPEN_WRITE_PLUS	6
#define SEMIHOSTING_OPEN_WRITE_PLUS_BIN	7
#define SEMIHOSTING_OPEN_APPEND		8
#define SEMIHOSTING_OPEN_APPEND_BIN	9
#define SEMIHOSTING_OPEN_APPEND_PLUS	10
#define SEMIHOSTING_OPEN_APPEND_PLUS_BIN 11

#define SEMIHOSTING_NO_HANDLE		((TSemihostingValue) -1)

#define SEMIHOSTING_STDIO_NAME		":tt"


void SemihostingExit (TSemihostingValue nStatusCode);

#ifdef __cplusplus
}
#endif

#endif
