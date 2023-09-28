//
// bcm2711int.h
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
#ifndef _circle_bcm2711int_h
#define _circle_bcm2711int_h

#if RASPPI >= 4

// conversion from device tree source
#define GIC_PPI(n)		(16 + (n))	// private per core
#define GIC_SPI(n)		(32 + (n))	// shared between cores

// IRQs
#define ARM_IRQLOCAL0_CNTPNS	GIC_PPI (14)

#define ARM_IRQ_ARM_DOORBELL_0	GIC_SPI (34)
#define ARM_IRQ_TIMER1		GIC_SPI (65)
#define ARM_IRQ_USB		GIC_SPI (73)
#define ARM_IRQ_DMA0		GIC_SPI (80)
#define ARM_IRQ_DMA1		GIC_SPI (81)
#define ARM_IRQ_DMA2		GIC_SPI (82)
#define ARM_IRQ_DMA3		GIC_SPI (83)
#define ARM_IRQ_DMA4		GIC_SPI (84)
#define ARM_IRQ_DMA5		GIC_SPI (85)
#define ARM_IRQ_DMA6		GIC_SPI (86)
#define ARM_IRQ_DMA7		GIC_SPI (87)
#define ARM_IRQ_DMA8		GIC_SPI (87)	// same value
#define ARM_IRQ_DMA9		GIC_SPI (88)
#define ARM_IRQ_DMA10		GIC_SPI (88)	// same value
#define ARM_IRQ_DMA11		GIC_SPI (89)
#define ARM_IRQ_DMA12		GIC_SPI (90)
#define ARM_IRQ_DMA13		GIC_SPI (91)
#define ARM_IRQ_DMA14		GIC_SPI (92)
#define ARM_IRQ_CAM0		GIC_SPI (102)
#define ARM_IRQ_CAM1		GIC_SPI (103)
#define ARM_IRQ_GPIO0		GIC_SPI (113)
#define ARM_IRQ_GPIO1		GIC_SPI (114)
#define ARM_IRQ_GPIO2		GIC_SPI (115)
#define ARM_IRQ_GPIO3		GIC_SPI (116)
#define ARM_IRQ_UART		GIC_SPI (121)
#define ARM_IRQ_ARASANSDIO	GIC_SPI (126)
#define ARM_IRQ_PCIE_HOST_INTA	GIC_SPI (143)
#define ARM_IRQ_PCIE_HOST_MSI	GIC_SPI (148)
#define ARM_IRQ_BCM54213_0	GIC_SPI (157)
#define ARM_IRQ_BCM54213_1	GIC_SPI (158)
#define ARM_IRQ_XHCI_INTERNAL	GIC_SPI (176)

#define IRQ_LINES		256

// FIQs
#define ARM_FIQ_TIMER1		ARM_IRQ_TIMER1
#define ARM_FIQ_GPIO3		ARM_IRQ_GPIO3
#define ARM_FIQ_UART		ARM_IRQ_UART

#define ARM_MAX_FIQ		IRQ_LINES

#endif

#endif
