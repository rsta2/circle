//
// xhciconfig.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019-2023  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_xhciconfig_h
#define _circle_usb_xhciconfig_h

#include <circle/sysconfig.h>

//#define XHCI_DEBUG
//#define XHCI_DEBUG2

//
// PCIe Configuration
//
#ifndef USE_XHCI_INTERNAL
#define XHCI_PCIE_BUS			1
#define XHCI_PCIE_SLOT			0
#define XHCI_PCIE_FUNC			0

#define XHCI_PCI_CLASS_CODE		0xC0330
#endif

//
// Driver Configuration
//
#ifdef USE_XHCI_INTERNAL
#define XHCI_CONFIG_MAX_SLOTS		64
#define XHCI_CONFIG_MAX_PORTS		1
#else
#define XHCI_CONFIG_MAX_SLOTS		32
#define XHCI_CONFIG_MAX_PORTS		5
#endif

#define XHCI_CONFIG_EVENT_RING_SIZE	256
#define XHCI_CONFIG_CMD_RING_SIZE	64
#define XHCI_CONFIG_TRANSFER_RING_SIZE	64

#define XHCI_CONFIG_IMODI		500		// defines maximum interrupt rate

#define XHCI_CONFIG_MAX_EVENTS_PER_INTR	16		// max. events to be handled per interrupt

#define XHCI_PAGE_SHIFT			12
#define XHCI_PAGE_SIZE			(1 << XHCI_PAGE_SHIFT)

#define XHCI_CONFIG_MAX_REQUESTS	(XHCI_CONFIG_MAX_SLOTS * 8)

//
// Port Configuration (from Extended Capabilities "Protocol")
//
// USB2: Port 1, HSO=1, IHI=1, HLC=0, PSIC=1 (PSIV=3, PSIE=2, PLT=0, PFD=0, PSIM=480)
// USB3: Port 2-5, PSIC=1 (PSIV=4, PSIE=3, PLT=0, PFD=1, PSIM=5)
//
// Internal controller:
// USB2: Port 1, HSO=0, IHI=0, HLC=1, BLC=1, MHD=0, PSIC=0
//
#define XHCI_IS_USB2_PORT(portid)	((portid) == 1)

// PSIs
#define XHCI_PORT_SPEED_FULL		1
#define XHCI_PORT_SPEED_LOW		2
#define XHCI_PORT_SPEED_HIGH		3
#define XHCI_PORT_SPEED_SUPER		4
#define XHCI_IS_PSI(num)		(   (num) >= XHCI_PORT_SPEED_FULL		\
					 && (num) <= XHCI_PORT_SPEED_SUPER)
#define XHCI_PSI_TO_USB_SPEED(psi)	((TUSBSpeed) (   (psi) < XHCI_PORT_SPEED_HIGH	\
						      ? ((psi) - 1) ^ 1 : (psi) - 1))
#define XHCI_USB_SPEED_TO_PSI(speed)	((unsigned) (  (speed) < USBSpeedHigh		\
						     ? ((speed) ^ 1) + 1 : (speed) + 1))

#endif
