//
// sysconfig.h
//
// Configurable system options
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
#ifndef _circle_sysconfig_h
#define _circle_sysconfig_h

///////////////////////////////////////////////////////////////////////
//
// Memory
//
///////////////////////////////////////////////////////////////////////

#define MEGABYTE		0x100000	// do not change

// KERNEL_MAX_SIZE is the maximum allowed size of a built kernel image.
// If your kernel image contains big data areas it may be required to
// increase this value. The value must be a multiple of 16 KByte.

#define KERNEL_MAX_SIZE		(2 * MEGABYTE)

///////////////////////////////////////////////////////////////////////
//
// Raspberry Pi 1 and Zero
//
///////////////////////////////////////////////////////////////////////

#if RASPPI == 1

// ARM_STRICT_ALIGNMENT activates the memory alignment check. If an
// unaligned memory access occurs an exception is raised with this
// option defined. This should normally not be activated, because
// newer Circle images are not tested with it.

//#define ARM_STRICT_ALIGNMENT

// GPU_L2_CACHE_ENABLED has to be defined, if the L2 cache of the GPU
// is enabled, which is normally the case. Only if you have disabled
// the L2 cache of the GPU in config.txt this option must be undefined.

#define GPU_L2_CACHE_ENABLED

// USE_PWM_AUDIO_ON_ZERO can be defined to use GPIO12/13 for PWM audio
// output on RPi Zero (W). Some external circuit is needed to use this.
// WARNING: Do not feed voltage into these GPIO pins with this option
//          defined on a RPi Zero, because this may destroy the pins.

//#define USE_PWM_AUDIO_ON_ZERO

#endif

///////////////////////////////////////////////////////////////////////
//
// Raspberry Pi 2, 3 and 4
//
///////////////////////////////////////////////////////////////////////

#if RASPPI >= 2

// USE_RPI_STUB_AT enables the debugging support for rpi_stub and
// defines the address where rpi_stub is loaded. See doc/debug.txt
// for details! Kernel images built with this option defined do
// normally not run without rpi_stub loaded.

//#define USE_RPI_STUB_AT 	0x1F000000

#ifndef USE_RPI_STUB_AT

// ARM_ALLOW_MULTI_CORE has to be defined to use multi-core support
// with the class CMultiCoreSupport. It should not be defined for
// single core applications, because this may slow down the system
// because multiple cores may compete for bus time without use.

//#define ARM_ALLOW_MULTI_CORE

#endif

// USE_PHYSICAL_COUNTER enables the use of the CPU internal physical
// counter, which is only available on the Raspberry Pi 2, 3 and 4. Reading
// this counter is much faster than reading the BCM2835 system timer
// counter (which is used without this option). It reduces the I/O load
// too. For some QEMU versions this is the only supported timer option,
// for other older QEMU versions it does not work. On the Raspberry Pi 4
// setting this option is required.

#define USE_PHYSICAL_COUNTER

#endif

///////////////////////////////////////////////////////////////////////
//
// Timing
//
///////////////////////////////////////////////////////////////////////

// REALTIME optimizes the IRQ latency of the system, which could be
// useful for time-critical applications. This will be accomplished
// by disabling some features (e.g. USB low-/full-speed device support).
// See doc/realtime.txt for details!

//#define REALTIME

#ifndef REALTIME

// USE_USB_SOF_INTR improves the compatibility with low-/full-speed
// USB devices. If your application uses such devices, this option
// should normally be set. Unfortunately this causes a heavily changed
// system timing, because it triggers up to 8000 IRQs per second. For
// compatibility with existing applications it is not set by default.
// This option has no influence on the Raspberry Pi 4.

//#define USE_USB_SOF_INTR

#endif

// SCREEN_DMA_BURST_LENGTH enables using DMA for scrolling the screen
// contents and set the burst length parameter for the DMA controller.
// Using DMA speeds up the scrolling, especially with a burst length
// greater than 0. The parameter can be 0-15 theoretically, but values
// over 2 are normally not useful, because the system bus gets congested
// with it.

#define SCREEN_DMA_BURST_LENGTH	2

// CALIBRATE_DELAY activates the calibration of the delay loop. Because
// this loop is normally not used any more in Circle, the only use of
// this option is that the "SpeedFactor" of your system is displayed.
// You can reduce the time needed for booting, if you disable this.

#define CALIBRATE_DELAY

///////////////////////////////////////////////////////////////////////
//
// Scheduler
//
///////////////////////////////////////////////////////////////////////

// MAX_TASKS is the maximum number of tasks in the system.

#define MAX_TASKS		20

// TASK_STACK_SIZE is the stack size for each task.

#define TASK_STACK_SIZE		0x8000

///////////////////////////////////////////////////////////////////////
//
// USB keyboard
//
///////////////////////////////////////////////////////////////////////

// DEFAULT_KEYMAP selects the default keyboard map (enable only one).
// The default keyboard map can be overwritten in with the keymap=
// option in cmdline.txt.

#define DEFAULT_KEYMAP		"DE"
//#define DEFAULT_KEYMAP		"ES"
//#define DEFAULT_KEYMAP		"FR"
//#define DEFAULT_KEYMAP		"IT"
//#define DEFAULT_KEYMAP		"UK"
//#define DEFAULT_KEYMAP		"US"

///////////////////////////////////////////////////////////////////////
//
// Other
//
///////////////////////////////////////////////////////////////////////

// SERIAL_GPIO_SELECT selects the TXD GPIO pin used for the serial
// device (UART0). The RXD pin is (SERIAL_GPIO_SELECT+1). Modifying
// this setting can be useful for Compute Modules. Select only one
// definition.

#define SERIAL_GPIO_SELECT	14	// and 15
//#define SERIAL_GPIO_SELECT	32	// and 33
//#define SERIAL_GPIO_SELECT	36	// and 37

// SAVE_VFP_REGS_ON_IRQ enables saving the floating point registers
// on entry when an IRQ occurs and will restore these registers on exit
// from the IRQ handler. This has to be defined, if an IRQ handler
// modifies floating point registers. IRQ handling will be a little
// slower then.

//#define SAVE_VFP_REGS_ON_IRQ

// SAVE_VFP_REGS_ON_FIQ enables saving the floating point registers
// on entry when an FIQ occurs and will restore these registers on exit
// from the FIQ handler. This has to be defined, if the FIQ handler
// modifies floating point registers. FIQ handling will be a little
// slower then.

//#define SAVE_VFP_REGS_ON_FIQ

// USE_QEMU_USB_FIX fixes an issue when using Circle images inside
// QEMU. If you encounter Circle freezing when using USB in QEMU
// you should activate this option. It must not be defined for
// Circle images which will run on real Raspberry Pi boards.

//#define USE_QEMU_USB_FIX

///////////////////////////////////////////////////////////////////////

#include <circle/memorymap.h>

#endif
