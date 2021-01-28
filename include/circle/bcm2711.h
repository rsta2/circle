//
// bcm2711.h
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
#ifndef _circle_bcm2711_h
#define _circle_bcm2711_h

#if RASPPI >= 4

#include <circle/bcm2835.h>
#include <circle/sysconfig.h>

//
// External Mass Media Controller 2 (SD Card)
//
#define ARM_EMMC2_BASE		(ARM_IO_BASE + 0x340000)

//
// Hardware Random Number Generator RNG200
//
#define ARM_HW_RNG200_BASE	(ARM_IO_BASE + 0x104000)

//
// Generic Interrupt Controller (GIC-400)
//
#define ARM_GICD_BASE		0xFF841000
#define ARM_GICC_BASE		0xFF842000
#define ARM_GIC_END		0xFF847FFF

//
// BCM54213PE Gigabit Ethernet Transceiver (external)
//
#define ARM_BCM54213_BASE	0xFD580000
#define ARM_BCM54213_MDIO	(ARM_BCM54213_BASE + 0x0E14)
#define ARM_BCM54213_MDIO_END	(ARM_BCM54213_BASE + 0x0E1B)
#define ARM_BCM54213_END	(ARM_BCM54213_BASE + 0xFFFF)

//
// PCIe Host Bridge
//
#define ARM_PCIE_HOST_BASE	0xFD500000
#define ARM_PCIE_HOST_END	(ARM_PCIE_HOST_BASE + 0x930F)

//
// xHCI USB Host Controller
//
#ifdef USE_XHCI_INTERNAL
	#define ARM_XHCI_BASE	(ARM_IO_BASE + 0x9C0000)
#else
	#define ARM_XHCI_BASE	MEM_PCIE_RANGE_START_VIRTUAL
#endif
#define ARM_XHCI_END		(ARM_XHCI_BASE + 0x0FFF)

#endif

#endif
