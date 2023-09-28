//
// bcm2835.h
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
#ifndef _circle_bcm2835_h
#define _circle_bcm2835_h

#include <circle/sysconfig.h>

#if RASPPI == 1
#define ARM_IO_BASE		0x20000000
#elif RASPPI <= 3
#define ARM_IO_BASE		0x3F000000
#else
#define ARM_IO_BASE		0xFE000000
#endif
#define ARM_IO_END		(ARM_IO_BASE + 0xFFFFFF)

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

// Convert ARM address to GPU bus address (does also work for aliases)
#define BUS_ADDRESS(addr)	(((addr) & ~0xC0000000) | GPU_MEM_BASE)

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
#if RASPPI <= 3
#define ARM_GPIO_GPPUD		(ARM_GPIO_BASE + 0x94)
#define ARM_GPIO_GPPUDCLK0	(ARM_GPIO_BASE + 0x98)
#else
#define ARM_GPIO_GPPINMUXSD	(ARM_GPIO_BASE + 0xD0)
#define ARM_GPIO_GPPUPPDN0	(ARM_GPIO_BASE + 0xE4)
#define ARM_GPIO_GPPUPPDN1	(ARM_GPIO_BASE + 0xE8)
#define ARM_GPIO_GPPUPPDN2	(ARM_GPIO_BASE + 0xEC)
#define ARM_GPIO_GPPUPPDN3	(ARM_GPIO_BASE + 0xF0)
#endif

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

#if RASPPI >= 4
#define ARM_PWM1_BASE		(ARM_IO_BASE + 0x20C800)

#define ARM_PWM1_CTL		(ARM_PWM1_BASE + 0x00)
#define ARM_PWM1_STA		(ARM_PWM1_BASE + 0x04)
#define ARM_PWM1_DMAC		(ARM_PWM1_BASE + 0x08)
#define ARM_PWM1_RNG1		(ARM_PWM1_BASE + 0x10)
#define ARM_PWM1_DAT1		(ARM_PWM1_BASE + 0x14)
#define ARM_PWM1_FIF1		(ARM_PWM1_BASE + 0x18)
#define ARM_PWM1_RNG2		(ARM_PWM1_BASE + 0x20)
#define ARM_PWM1_DAT2		(ARM_PWM1_BASE + 0x24)
#endif

//
// Clock Manager
//
#define ARM_CM_BASE		(ARM_IO_BASE + 0x101000)

#define ARM_CM_CAM0CTL		(ARM_CM_BASE + 0x40)
#define ARM_CM_CAM0DIV		(ARM_CM_BASE + 0x44)
#define ARM_CM_CAM1CTL		(ARM_CM_BASE + 0x48)
#define ARM_CM_CAM1DIV		(ARM_CM_BASE + 0x4C)

#define ARM_CM_GP0CTL		(ARM_CM_BASE + 0x70)
#define ARM_CM_GP0DIV		(ARM_CM_BASE + 0x74)

#define ARM_CM_SMICTL		(ARM_CM_BASE + 0xB0)
#define ARM_CM_SMIDIV		(ARM_CM_BASE + 0xB4)

#define ARM_CM_PASSWD 		(0x5A << 24)

//
// USB Host Controller
//
#define ARM_USB_BASE		(ARM_IO_BASE + 0x980000)

#define ARM_USB_CORE_BASE	ARM_USB_BASE
#define ARM_USB_HOST_BASE	(ARM_USB_BASE + 0x400)
#define ARM_USB_DEV_BASE	(ARM_USB_BASE + 0x800)
#define ARM_USB_POWER		(ARM_USB_BASE + 0xE00)

//
// Host Port (MPHI)
//
#define ARM_MPHI_BASE		(ARM_IO_BASE + 0x6000)
#define ARM_MPHI_END		(ARM_IO_BASE + 0x6FFF)

#define ARM_MPHI_OUTDDA		(ARM_MPHI_BASE + 0x28)
#define ARM_MPHI_OUTDDB		(ARM_MPHI_BASE + 0x2C)
#define ARM_MPHI_CTRL		(ARM_MPHI_BASE + 0x4C)
#define ARM_MPHI_INTSTAT	(ARM_MPHI_BASE + 0x50)

//
// External Mass Media Controller (SD Card)
//
#define ARM_EMMC_BASE		(ARM_IO_BASE + 0x300000)

//
// SDHOST Controller (SD Card)
//
#define ARM_SDHOST_BASE		(ARM_IO_BASE + 0x202000)

//
// Power Manager
//
#define ARM_PM_BASE		(ARM_IO_BASE + 0x100000)

#define ARM_PM_RSTC		(ARM_PM_BASE + 0x1C)
#define ARM_PM_RSTS		(ARM_PM_BASE + 0x20)
#define ARM_PM_WDOG		(ARM_PM_BASE + 0x24)
#define ARM_PM_PADS0		(ARM_PM_BASE + 0x2C)    // GPIO 0 - 27
#define ARM_PM_PADS1		(ARM_PM_BASE + 0x30)    // GPIO 28 - 45
#define ARM_PM_PADS2		(ARM_PM_BASE + 0x34)    // GPIO 46 - 53

#define ARM_PM_PASSWD		(0x5A << 24)
#define ARM_PM_RSTC_CLEAR	0xFFFFFFCF
#define ARM_PM_RSTC_REBOOT	0x00000020
#define ARM_PM_RSTC_RESET	0x00000102
#define ARM_PM_RSTS_PART_CLEAR	0xFFFFFAAA
#define ARM_PM_WDOG_TIME	0x000FFFFF
#define ARM_PADS_SLEW		(0x01 << 4)
#define ARM_PADS_HYST		(0x01 << 3)
#define ARM_PADS_DRIVE(val)	((val) & 0x3)

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
// Auxiliary Peripherals
//
#define ARM_AUX_BASE		(ARM_IO_BASE + 0x215000)

#define ARM_AUX_ENABLE		(ARM_AUX_BASE + 0x04)
	#define ARM_AUX_ENABLE_SPI1	0x02

#define ARM_AUX_SPI0_CNTL0	(ARM_AUX_BASE + 0x80)
#define ARM_AUX_SPI0_CNTL1	(ARM_AUX_BASE + 0x84)
#define ARM_AUX_SPI0_STAT	(ARM_AUX_BASE + 0x88)
#define ARM_AUX_SPI0_IO		(ARM_AUX_BASE + 0xA0)
#define ARM_AUX_SPI0_TXHOLD	(ARM_AUX_BASE + 0xB0)

//
// Hardware Random Number Generator
//
#define ARM_HW_RNG_BASE		(ARM_IO_BASE + 0x104000)

#define ARM_HW_RNG_CTRL		(ARM_HW_RNG_BASE + 0x00)
	#define ARM_HW_RNG_CTRL_EN	0x01
#define ARM_HW_RNG_STATUS	(ARM_HW_RNG_BASE + 0x04)
#define ARM_HW_RNG_DATA		(ARM_HW_RNG_BASE + 0x08)

//
// PCM / I2S Audio Module
//
#define ARM_PCM_BASE		(ARM_IO_BASE + 0x203000)

#define ARM_PCM_CS_A		(ARM_PCM_BASE + 0x00)
#define ARM_PCM_FIFO_A		(ARM_PCM_BASE + 0x04)
#define ARM_PCM_MODE_A		(ARM_PCM_BASE + 0x08)
#define ARM_PCM_RXC_A		(ARM_PCM_BASE + 0x0C)
#define ARM_PCM_TXC_A		(ARM_PCM_BASE + 0x10)
#define ARM_PCM_DREQ_A		(ARM_PCM_BASE + 0x14)
#define ARM_PCM_INTEN_A		(ARM_PCM_BASE + 0x18)
#define ARM_PCM_INTSTC_A	(ARM_PCM_BASE + 0x1C)
#define ARM_PCM_GRAY		(ARM_PCM_BASE + 0x20)

//
// VC4 VCHIQ
//
#define ARM_VCHIQ_BASE		(ARM_IO_BASE + 0xB840)
#define ARM_VCHIQ_END		(ARM_VCHIQ_BASE + 0x0F)

//
// VC4/5 HDMI
//
#if RASPPI <= 3
#define ARM_HDMI_BASE		(ARM_IO_BASE + 0x902000)
#define ARM_HD_BASE		(ARM_IO_BASE + 0x808000)
#else
#define ARM_HDMI_BASE		(ARM_IO_BASE + 0xF00700)
#define ARM_HD_BASE		(ARM_IO_BASE + 0xF20000)
#define ARM_PHY_BASE		(ARM_IO_BASE + 0xF00F00)
#define ARM_RAM_BASE		(ARM_IO_BASE + 0xF01B00)
#endif

//
// CSI
//
#define ARM_CSI0_BASE		(ARM_IO_BASE + 0x800000)
#define ARM_CSI0_END		(ARM_CSI0_BASE + 0x7FF)
#define ARM_CSI0_CLKGATE	(ARM_IO_BASE + 0x802000)	// 4 bytes

#define ARM_CSI1_BASE		(ARM_IO_BASE + 0x801000)
#define ARM_CSI1_END		(ARM_CSI1_BASE + 0x7FF)
#define ARM_CSI1_CLKGATE	(ARM_IO_BASE + 0x802004)	// 4 bytes

#endif
