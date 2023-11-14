//
// bcm2712.h
//
// Memory I/O addresses of peripherals, which were introduced
// with the Raspberry Pi 5
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_bcm2712_h
#define _circle_bcm2712_h

#if RASPPI >= 5

#include <circle/bcm2835.h>

//
// General Purpose I/O #2
//
#define ARM_GPIO2_BASE		(ARM_IO_BASE + 0x1517C00)

#define ARM_GPIO2_DATA0		(ARM_GPIO2_BASE + 0x04)
#define ARM_GPIO2_IODIR0	(ARM_GPIO2_BASE + 0x08)

#endif

#endif
