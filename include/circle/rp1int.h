//
// rp1int.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023-2024  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_rp1int_h
#define _circle_rp1int_h

#if RASPPI >= 5

// Bit masks for the general IRQ numbers
#define IRQ_NUMBER__MASK	0x3FF
#define IRQ_EDGE_TRIG__MASK	0x400		// level triggered otherwise
#define IRQ_FROM_RP1__MASK	0x800		// second level IRQ via RP1

#define RP1_IRQ_LEVEL(n)	((n) | IRQ_FROM_RP1__MASK)
#define RP1_IRQ_EDGE(n)		((n) | IRQ_FROM_RP1__MASK | IRQ_EDGE_TRIG__MASK)

#define RP1_IRQ_IO_BANK0	RP1_IRQ_LEVEL (0)
#define RP1_IRQ_IO_BANK1	RP1_IRQ_LEVEL (1)
#define RP1_IRQ_IO_BANK2	RP1_IRQ_LEVEL (2)
#define RP1_IRQ_ETH		RP1_IRQ_LEVEL (6)
#define RP1_IRQ_I2S0		RP1_IRQ_LEVEL (14)
#define RP1_IRQ_I2S1		RP1_IRQ_LEVEL (15)
#define RP1_IRQ_UART0		RP1_IRQ_LEVEL (25)
#define RP1_IRQ_USBHOST0	RP1_IRQ_EDGE (30)
#define RP1_IRQ_USBHOST0_0	RP1_IRQ_EDGE (31)
#define RP1_IRQ_USBHOST1	RP1_IRQ_EDGE (35)
#define RP1_IRQ_USBHOST1_0	RP1_IRQ_EDGE (36)
#define RP1_IRQ_DMA		RP1_IRQ_LEVEL (40)
#define RP1_IRQ_UART1		RP1_IRQ_LEVEL (42)
#define RP1_IRQ_UART2		RP1_IRQ_LEVEL (43)
#define RP1_IRQ_UART3		RP1_IRQ_LEVEL (44)
#define RP1_IRQ_UART4		RP1_IRQ_LEVEL (45)
#define RP1_IRQ_UART5		RP1_IRQ_LEVEL (46)

#define RP1_IRQ_LINES		61

#endif

#endif
