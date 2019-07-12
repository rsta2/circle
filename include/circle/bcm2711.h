//
// bcm2711.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2019  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_bcm2711_h
#define _circle_bcm2711_h

#if RASPPI >= 4

//
// Generic Interrupt Controller (GIC-400)
//
#define ARM_GICD_BASE		0xFF841000
#define ARM_GICC_BASE		0xFF842000
#define ARM_GIC_END		0xFF847FFF

#endif

#endif
