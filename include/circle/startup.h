//
// startup.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2023  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_startup_h
#define _circle_startup_h

#include <circle/sysconfig.h>
#include <circle/macros.h>
#include <circle/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int main (void);
#define EXIT_HALT	0
#define EXIT_REBOOT	1

void sysinit (void) NORETURN;

void halt (void) NORETURN;
void reboot (void) NORETURN;

void set_qemu_exit_status (int nStatus);
#define EXIT_STATUS_SUCCESS	0
#define EXIT_STATUS_PANIC	255

#if RASPPI != 1

void _start_secondary (void);

#ifdef ARM_ALLOW_MULTI_CORE

void main_secondary (void);

#if AARCH == 64

struct TSpinTable
{
	uintptr SpinCore[CORES];
}
PACKED;

#define ARM_SPIN_TABLE_BASE	0x000000D8

#endif

#endif

#endif

#define ARM_DTB_PTR32		0x000000F8

#ifdef __cplusplus
}
#endif

#endif
