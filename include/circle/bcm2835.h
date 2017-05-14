//
// bcm2835.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_bcm2835_h
#define _circle_bcm2835_h

#include <circle/sysconfig.h>

#if RASPPI == 1
#define ARM_IO_BASE		0x20000000
#else
#define ARM_IO_BASE		0x3F000000
#endif

#define GPU_IO_BASE		0x7E000000

#define GPU_CACHED_BASE		0x40000000
#define GPU_UNCACHED_BASE	0xC0000000

#if RASPPI == 1
	#ifdef GPU_L2_CACHE_ENABLED
		#define GPU_MEM_BASE	GPU_CACHED_BASE
	#else
		#define GPU_MEM_BASE	GPU_UNCACHED_BASE
	#endif
#else
	#define GPU_MEM_BASE	GPU_UNCACHED_BASE
#endif

//
// General Purpose I/O
//
#define ARM_GPIO_BASE		(ARM_IO_BASE + 0x200000)

#define ARM_GPIO_GPFSEL0	(ARM_GPIO_BASE + 0x00)
#define ARM_GPIO_GPFSEL1	(ARM_GPIO_BASE + 0x04)
#define ARM_GPIO_GPFSEL4	(ARM_GPIO_BASE + 0x10)
#define ARM_GPIO_GPSET0		(ARM_GPIO_BASE + 0x1C)
#define ARM_GPIO_GPCLR0		(ARM_GPIO_BASE + 0x28)
#define ARM_GPIO_GPLEV0		(ARM_GPIO_BASE + 0x34)
#define ARM_GPIO_GPEDS0		(ARM_GPIO_BASE + 0x40)
#define ARM_GPIO_GPREN0		(ARM_GPIO_BASE + 0x4C)
#define ARM_GPIO_GPFEN0		(ARM_GPIO_BASE + 0x58)
#define ARM_GPIO_GPHEN0		(ARM_GPIO_BASE + 0x64)
#define ARM_GPIO_GPLEN0		(ARM_GPIO_BASE + 0x70)
#define ARM_GPIO_GPAREN0	(ARM_GPIO_BASE + 0x7C)
#define ARM_GPIO_GPAFEN0	(ARM_GPIO_BASE + 0x88)
#define ARM_GPIO_GPPUD		(ARM_GPIO_BASE + 0x94)
#define ARM_GPIO_GPPUDCLK0	(ARM_GPIO_BASE + 0x98)

//
// UART0
//
#define ARM_UART0_BASE		(ARM_IO_BASE + 0x201000)

#define ARM_UART0_DR		(ARM_UART0_BASE + 0x00)
#define ARM_UART0_FR     	(ARM_UART0_BASE + 0x18)
#define ARM_UART0_IBRD   	(ARM_UART0_BASE + 0x24)
#define ARM_UART0_FBRD   	(ARM_UART0_BASE + 0x28)
#define ARM_UART0_LCRH   	(ARM_UART0_BASE + 0x2C)
#define ARM_UART0_CR     	(ARM_UART0_BASE + 0x30)
#define ARM_UART0_IFLS   	(ARM_UART0_BASE + 0x34)
#define ARM_UART0_IMSC   	(ARM_UART0_BASE + 0x38)
#define ARM_UART0_RIS    	(ARM_UART0_BASE + 0x3C)
#define ARM_UART0_MIS    	(ARM_UART0_BASE + 0x40)
#define ARM_UART0_ICR    	(ARM_UART0_BASE + 0x44)

//
// System Timers
//
#define ARM_SYSTIMER_BASE	(ARM_IO_BASE + 0x3000)

#define ARM_SYSTIMER_CS		(ARM_SYSTIMER_BASE + 0x00)
#define ARM_SYSTIMER_CLO	(ARM_SYSTIMER_BASE + 0x04)
#define ARM_SYSTIMER_CHI	(ARM_SYSTIMER_BASE + 0x08)
#define ARM_SYSTIMER_C0		(ARM_SYSTIMER_BASE + 0x0C)
#define ARM_SYSTIMER_C1		(ARM_SYSTIMER_BASE + 0x10)
#define ARM_SYSTIMER_C2		(ARM_SYSTIMER_BASE + 0x14)
#define ARM_SYSTIMER_C3		(ARM_SYSTIMER_BASE + 0x18)

//
// Platform DMA Controller
//
#define ARM_DMA_BASE		(ARM_IO_BASE + 0x7000)

//
// Interrupt Controller
//
#define ARM_IC_BASE		(ARM_IO_BASE + 0xB000)

#define ARM_IC_IRQ_BASIC_PENDING  (ARM_IC_BASE + 0x200)
#define ARM_IC_IRQ_PENDING_1	  (ARM_IC_BASE + 0x204)
#define ARM_IC_IRQ_PENDING_2	  (ARM_IC_BASE + 0x208)
#define ARM_IC_FIQ_CONTROL	  (ARM_IC_BASE + 0x20C)
#define ARM_IC_ENABLE_IRQS_1	  (ARM_IC_BASE + 0x210)
#define ARM_IC_ENABLE_IRQS_2	  (ARM_IC_BASE + 0x214)
#define ARM_IC_ENABLE_BASIC_IRQS  (ARM_IC_BASE + 0x218)
#define ARM_IC_DISABLE_IRQS_1	  (ARM_IC_BASE + 0x21C)
#define ARM_IC_DISABLE_IRQS_2	  (ARM_IC_BASE + 0x220)
#define ARM_IC_DISABLE_BASIC_IRQS (ARM_IC_BASE + 0x224)

//
// ARM Timer
//
#define ARM_TIMER_BASE		(ARM_IO_BASE + 0xB000)

#define ARM_TIMER_LOAD		(ARM_TIMER_BASE + 0x400)
#define ARM_TIMER_VALUE		(ARM_TIMER_BASE + 0x404)
#define ARM_TIMER_CTRL		(ARM_TIMER_BASE + 0x408)
#define ARM_TIMER_IRQCLR	(ARM_TIMER_BASE + 0x40C)
#define ARM_TIMER_RAWIRQ	(ARM_TIMER_BASE + 0x410)
#define ARM_TIMER_MASKIRQ	(ARM_TIMER_BASE + 0x414)
#define ARM_TIMER_RELOAD	(ARM_TIMER_BASE + 0x418)
#define ARM_TIMER_PREDIV	(ARM_TIMER_BASE + 0x41C)
#define ARM_TIMER_CNTR		(ARM_TIMER_BASE + 0x420)

//
// Mailbox
//
#define MAILBOX_BASE		(ARM_IO_BASE + 0xB880)

#define MAILBOX0_READ  		(MAILBOX_BASE + 0x00)
#define MAILBOX0_STATUS 	(MAILBOX_BASE + 0x18)
	#define MAILBOX_STATUS_EMPTY	0x40000000
#define MAILBOX1_WRITE		(MAILBOX_BASE + 0x20)
#define MAILBOX1_STATUS 	(MAILBOX_BASE + 0x38)
	#define MAILBOX_STATUS_FULL	0x80000000

#define MAILBOX_CHANNEL_PM	0			// power management
#define MAILBOX_CHANNEL_FB 	1			// frame buffer
#define BCM_MAILBOX_PROP_OUT	8			// property tags (ARM to VC)

//
// Pulse Width Modulator
//
#define ARM_PWM_BASE		(ARM_IO_BASE + 0x20C000)

#define ARM_PWM_CTL		(ARM_PWM_BASE + 0x00)
#define ARM_PWM_STA		(ARM_PWM_BASE + 0x04)
#define ARM_PWM_DMAC		(ARM_PWM_BASE + 0x08)
#define ARM_PWM_RNG1		(ARM_PWM_BASE + 0x10)
#define ARM_PWM_DAT1		(ARM_PWM_BASE + 0x14)
#define ARM_PWM_FIF1		(ARM_PWM_BASE + 0x18)
#define ARM_PWM_RNG2		(ARM_PWM_BASE + 0x20)
#define ARM_PWM_DAT2		(ARM_PWM_BASE + 0x24)

//
// Clock Manager
//
#define ARM_CM_BASE		(ARM_IO_BASE + 0x101000)

#define ARM_CM_GP0CTL		(ARM_CM_BASE + 0x70)
#define ARM_CM_GP0DIV		(ARM_CM_BASE + 0x74)

#define ARM_CM_PASSWD 		(0x5A << 24)

//
// USB Host Controller
//
#define ARM_USB_BASE		(ARM_IO_BASE + 0x980000)

#define ARM_USB_CORE_BASE	ARM_USB_BASE
#define ARM_USB_HOST_BASE	(ARM_USB_BASE + 0x400)
#define ARM_USB_POWER		(ARM_USB_BASE + 0xE00)

//
// External Mass Media Controller (SD Card)
//
#define ARM_EMMC_BASE		(ARM_IO_BASE + 0x300000)

//
// Power Manager (?)
//
#define ARM_PM_BASE		(ARM_IO_BASE + 0x100000)

#define ARM_PM_RSTC		(ARM_PM_BASE + 0x1C)
#define ARM_PM_WDOG		(ARM_PM_BASE + 0x24)

#define ARM_PM_PASSWD		(0x5A << 24)

//
// BSC Master
//
#define ARM_BSC0_BASE		(ARM_IO_BASE + 0x205000)
#define ARM_BSC1_BASE		(ARM_IO_BASE + 0x804000)

#define ARM_BSC_C__OFFSET	0x00
#define ARM_BSC_S__OFFSET	0x04
#define ARM_BSC_DLEN__OFFSET	0x08
#define ARM_BSC_A__OFFSET	0x0C
#define ARM_BSC_FIFO__OFFSET	0x10
#define ARM_BSC_DIV__OFFSET	0x14
#define ARM_BSC_DEL__OFFSET	0x18
#define ARM_BSC_CLKT__OFFSET	0x1C

//
// SPI0 Master
//
#define ARM_SPI0_BASE		(ARM_IO_BASE + 0x204000)

#define ARM_SPI0_CS		(ARM_SPI0_BASE + 0x00)
#define ARM_SPI0_FIFO		(ARM_SPI0_BASE + 0x04)
#define ARM_SPI0_CLK		(ARM_SPI0_BASE + 0x08)
#define ARM_SPI0_DLEN		(ARM_SPI0_BASE + 0x0C)
#define ARM_SPI0_LTOH		(ARM_SPI0_BASE + 0x10)
#define ARM_SPI0_DC		(ARM_SPI0_BASE + 0x14)

//
// BSC / SPI Slave
//
#define ARM_BSC_SPI_SLAVE_BASE	(ARM_IO_BASE + 0x214000)

#define ARM_BSC_SPI_SLAVE_DR	(ARM_BSC_SPI_SLAVE_BASE + 0x00)
#define ARM_BSC_SPI_SLAVE_RSR	(ARM_BSC_SPI_SLAVE_BASE + 0x04)
#define ARM_BSC_SPI_SLAVE_SLV	(ARM_BSC_SPI_SLAVE_BASE + 0x08)
#define ARM_BSC_SPI_SLAVE_CR	(ARM_BSC_SPI_SLAVE_BASE + 0x0C)
#define ARM_BSC_SPI_SLAVE_FR	(ARM_BSC_SPI_SLAVE_BASE + 0x10)
#define ARM_BSC_SPI_SLAVE_IFLS	(ARM_BSC_SPI_SLAVE_BASE + 0x14)
#define ARM_BSC_SPI_SLAVE_IMSC	(ARM_BSC_SPI_SLAVE_BASE + 0x18)
#define ARM_BSC_SPI_SLAVE_RIS	(ARM_BSC_SPI_SLAVE_BASE + 0x1C)
#define ARM_BSC_SPI_SLAVE_MIS	(ARM_BSC_SPI_SLAVE_BASE + 0x20)
#define ARM_BSC_SPI_SLAVE_ICR	(ARM_BSC_SPI_SLAVE_BASE + 0x24)

//
// Hardware Random Number Generator
//
#define ARM_HW_RNG_BASE		(ARM_IO_BASE + 0x104000)

#define ARM_HW_RNG_CTRL		(ARM_HW_RNG_BASE + 0x00)
	#define ARM_HW_RNG_CTRL_EN	0x01
#define ARM_HW_RNG_STATUS	(ARM_HW_RNG_BASE + 0x04)
#define ARM_HW_RNG_DATA		(ARM_HW_RNG_BASE + 0x08)

#endif
