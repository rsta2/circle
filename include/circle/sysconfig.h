//
// sysconfig.h
//
// Configurable system options
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

#ifndef KERNEL_MAX_SIZE
#define KERNEL_MAX_SIZE		(2 * MEGABYTE)
#endif

// KERNEL_STACK_SIZE is the size of the stack set on startup for the
// main thread.  This must be a multiple of 16 KByte.

#ifndef KERNEL_STACK_SIZE
#define KERNEL_STACK_SIZE	0x20000
#endif

// HEAP_DEFAULT_NEW defines the default heap to be used for the "new"
// operator, if a memory type is not explicitly specified. Possible
// values are HEAP_LOW (memory below 1 GByte), HEAP_HIGH (memory above
// 1 GByte) or HEAP_ANY (memory above 1 GB, if available, or memory
// below 1 GB otherwise). This value can be set to HEAP_ANY for
// a virtually unified heap, which uses the whole available memory
// space. Because this may cause problems with some devices, which
// explicitly need low memory for DMA, this value defaults to HEAP_LOW.
// This setting is only of importance for the Raspberry Pi 4.

#ifndef HEAP_DEFAULT_NEW
#define HEAP_DEFAULT_NEW	HEAP_LOW
#endif

// HEAP_DEFAULT_MALLOC defines the heap to be used for malloc() and
// calloc() calls. See the description of HEAP_DEFAULT_NEW for details!
// Modifying this setting is not recommended, because there are device
// drivers, which require to allocate low memory for DMA purpose using
// malloc(). This setting is only of importance for the Raspberry Pi 4.

#ifndef HEAP_DEFAULT_MALLOC
#define HEAP_DEFAULT_MALLOC	HEAP_LOW
#endif

// HEAP_BLOCK_BUCKET_SIZES configures the heap allocator, which is the
// base of dynamic memory management ("new" operator and malloc()). The
// heap allocator manages free memory blocks in a number of free lists
// (buckets). Each free list contains blocks of a specific size. On
// block allocation the requested block size is rounded up to the
// size of next available bucket size. If the requested size is greater
// than the largest available bucket size, the block can be allocated,
// but the memory space is lost, if the block will be freed later.
// Because the block buckets have to be walked through on each allocate
// and free operation, it is preferable to have only a few buckets.
// With this option you can configure the bucket sizes, so that they
// fit best for your application needs. You have to define a comma
// separated list of increasing bucket sizes. All sizes must be a
// multiple of 64. Up to 20 sizes can be defined.

#ifndef HEAP_BLOCK_BUCKET_SIZES
#define HEAP_BLOCK_BUCKET_SIZES	0x40,0x400,0x1000,0x4000,0x10000,0x40000,0x80000
#endif

///////////////////////////////////////////////////////////////////////
//
// Raspberry Pi 1, Zero (W) and Zero 2 W
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

#ifndef NO_GPU_L2_CACHE_ENABLED
#define GPU_L2_CACHE_ENABLED
#endif

#endif

#if RASPPI == 1 || RASPPI == 3

// USE_PWM_AUDIO_ON_ZERO can be defined to use GPIO12/13 (or 18/19) for
// PWM audio output on RPi Zero (W) and Zero 2 W. Some external circuit
// is needed to use this.
// WARNING: Do not feed voltage into these GPIO pins with this option
//          defined on a RPi Zero, because this may destroy the pins.

//#define USE_PWM_AUDIO_ON_ZERO

// The left PWM audio output pin is by default GPIO12. The following
// define moves it to GPIO18.
//#define USE_GPIO18_FOR_LEFT_PWM_ON_ZERO

// The right PWM audio output pin is by default GPIO13. The following
// define moves it to GPIO19.
//#define USE_GPIO19_FOR_RIGHT_PWM_ON_ZERO

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

#ifndef NO_PHYSICAL_COUNTER
#define USE_PHYSICAL_COUNTER
#endif

#endif

#if RASPPI >= 4

// USE_XHCI_INTERNAL enables the xHCI controller, which is integrated
// into the BCM2711 SoC. The Raspberry Pi 4 provides two independent
// xHCI USB host controllers, an external controller, which is connected
// to the four USB-A sockets (USB 3.0 and 2.0) and an internal controller,
// which is connected to the USB-C power socket (USB 2.0 only). By default
// Circle uses the external xHCI controller. If you want to use the
// internal controller instead, this option has to be defined. Enabling
// this option is the only possibility to use USB on the Compute Module 4
// with Circle. This setting requires the option "otg_mode=1" set in the
// config.txt file too!

//#define USE_XHCI_INTERNAL

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

// USE_USB_SOF_INTR improves the compatibility with low-/full-speed
// USB devices. If your application uses such devices, this option
// should normally be set. Unfortunately this causes a heavily changed
// system timing, because it triggers up to 8000 IRQs per second. For
// USB plug-and-play operation this option must be set in any case.
// This option has no influence on the Raspberry Pi 4.

#ifndef NO_USB_SOF_INTR
#define USE_USB_SOF_INTR
#endif

// USE_USB_FIQ makes the USB timing more accurate, by using the FIQ to
// handle time-critical interrupts from the USB controller, which are
// triggered 8000 times per second. When using the default IRQ instead,
// USB interrupts may be delayed or entire micro-frames may be skipped,
// when other IRQs are currently handled, which could result in
// communication problems with some USB devices. If this option is
// enabled, USE_USB_SOF_INTR will be enabled too, and the FIQ cannot be
// used for other purposes. This option has no influence on the
// Raspberry Pi 4.

//#define USE_USB_FIQ

#ifdef USE_USB_FIQ
#define USE_USB_SOF_INTR
#endif

// SCREEN_DMA_BURST_LENGTH enables using DMA for scrolling the screen
// contents and set the burst length parameter for the DMA controller.
// Using DMA speeds up the scrolling, especially with a burst length
// greater than 0. The parameter can be 0-15 theoretically, but values
// over 2 are normally not useful, because the system bus gets congested
// with it.

#ifndef SCREEN_DMA_BURST_LENGTH
#define SCREEN_DMA_BURST_LENGTH	2
#endif

// CALIBRATE_DELAY activates the calibration of the delay loop. Because
// this loop is normally not used any more in Circle, the only use of
// this option is that the "SpeedFactor" of your system is displayed.
// You can reduce the time needed for booting, if you disable this.

#ifndef NO_CALIBRATE_DELAY
#define CALIBRATE_DELAY
#endif

///////////////////////////////////////////////////////////////////////
//
// Scheduler
//
///////////////////////////////////////////////////////////////////////

// MAX_TASKS is the maximum number of tasks in the system.

#ifndef MAX_TASKS
#define MAX_TASKS		20
#endif

// TASK_STACK_SIZE is the stack size for each task.

#ifndef TASK_STACK_SIZE
#define TASK_STACK_SIZE		0x8000
#endif

// NO_BUSY_WAIT deactivates busy waiting in the EMMC, SDHOST and USB
// drivers, while waiting for the completion of a synchronous transfer.
// This requires the scheduler in the system and transfers must not be
// initiated from a secondary CPU core, when this option is enabled.

//#define NO_BUSY_WAIT

///////////////////////////////////////////////////////////////////////
//
// USB keyboard
//
///////////////////////////////////////////////////////////////////////

// DEFAULT_KEYMAP selects the default keyboard map (enable only one).
// The default keyboard map can be overwritten in with the keymap=
// option in cmdline.txt.

#ifndef DEFAULT_KEYMAP

#define DEFAULT_KEYMAP		"DE"
//#define DEFAULT_KEYMAP		"ES"
//#define DEFAULT_KEYMAP		"FR"
//#define DEFAULT_KEYMAP		"IT"
//#define DEFAULT_KEYMAP		"UK"
//#define DEFAULT_KEYMAP		"US"

#endif

///////////////////////////////////////////////////////////////////////
//
// USB gadgets
//
///////////////////////////////////////////////////////////////////////

// USB_GADGET_VENDOR_ID is the Vendor ID, which is used for your USB
// gadgets. Normally new USB Vendor IDs will be assigned by the USB-IF
// (https://usb.org/getting-vendor-id). For tests a unique free Vendor
// ID may be used. A list of known Vendor IDs can be found here:
// http://www.linux-usb.org/usb-ids.html. You must not use the same
// Vendor/Device ID combination for USB devices with different
// configurations. Especially on Windows hosts this may lead to
// malfunction. The default Vendor ID 0x0000 given here, is not a valid
// ID and will be rejected by the Circle USB gadget driver. You have to
// define a new one.

#ifndef USB_GADGET_VENDOR_ID
#define USB_GADGET_VENDOR_ID		0x0000
#endif

// USB_GADGET_DEVICE_ID_BASE is the base value for the assignment of
// USB Device IDs for USB gadgets in Circle. Used Device IDs start with
// USB_GADGET_DEVICE_ID_BASE and end with USB_GADGET_DEVICE_ID_BASE+N-1
// where N is the number of supported USB gadget devices in Circle.
// Be sure that there is no collision with other USB devices with the
// same USB Vendor ID!

#ifndef USB_GADGET_DEVICE_ID_BASE
#define USB_GADGET_DEVICE_ID_BASE	0x8001
#endif

///////////////////////////////////////////////////////////////////////
//
// Other
//
///////////////////////////////////////////////////////////////////////

// SCREEN_HEADLESS can be defined, if your Raspberry Pi runs without
// a display connected. Most Circle sample programs normally expect
// a display connected to work, but some can be used without a display
// available. Historically the screen initialization was working even
// without a display connected, without returning an error, but
// especially on the Raspberry Pi 4 this is not the case any more. Here
// it is required to define this option, otherwise the program
// initialization will fail without notice. In your own headless
// applications you should just not use the CScreenDevice class instead
// and direct the logger output to CSerialDevice or CNullDevice.

//#define SCREEN_HEADLESS

// USE_LOG_COLORS enables the use of different ANSI colors for different
// severities in the system log (bright red for LogPanic, bright magenta
// for LogError, bright yellow for LogWarning, bright white for LogNotice
// and LogDebug). All log messages are bright white, when this option is
// disabled, except LogPanic, which is bright red too.

//#define USE_LOG_COLORS

// SERIAL_GPIO_SELECT selects the TXD GPIO pin used for the serial
// device (UART0). The RXD pin is (SERIAL_GPIO_SELECT+1). Modifying
// this setting can be useful for Compute Modules. Select only one
// definition.

#ifndef SERIAL_GPIO_SELECT

#define SERIAL_GPIO_SELECT	14	// and 15
//#define SERIAL_GPIO_SELECT	32	// and 33
//#define SERIAL_GPIO_SELECT	36	// and 37

#endif

// USE_EMBEDDED_MMC_CM enables access to the on-board embedded MMC
// memory on Compute Modules 3+ and 4. Does not work with SD card on
// CM3+ Lite and CM4 Lite.

//#define USE_EMBEDDED_MMC_CM

// USE_SDHOST selects the SDHOST device as interface for SD card
// access. Otherwise the EMMC device is used for this purpose. The
// SDHOST device is supported by Raspberry Pi 1-3 and Zero, but
// not by QEMU. If you rely on a small IRQ latency, USE_SDHOST should
// be disabled.

#if RASPPI <= 3 && !defined (REALTIME) && !defined (USE_EMBEDDED_MMC_CM)

#ifndef NO_SDHOST
#define USE_SDHOST
#endif

#endif

// SD_HIGH_SPEED enables the high-speed extensions of the SD card
// driver, which should result in a better performance with modern SD
// cards. This is not tested that widely like the standard driver, why
// it is presented as an option here, but is enabled by default.

#ifndef NO_SD_HIGH_SPEED
#define SD_HIGH_SPEED
#endif

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

// LEAVE_QEMU_ON_HALT can be defined to exit QEMU when halt() is
// called or main() returns EXIT_HALT. QEMU has to be started with the
// -semihosting option, so that this works. This option must not be
// defined for Circle images which will run on real Raspberry Pi boards.

//#define LEAVE_QEMU_ON_HALT

// USE_QEMU_USB_FIX fixes an issue when using Circle images inside
// QEMU. If you encounter Circle freezing when using USB in QEMU
// you should activate this option. It must not be defined for
// Circle images which will run on real Raspberry Pi boards.

//#define USE_QEMU_USB_FIX

///////////////////////////////////////////////////////////////////////

// GNU-C 12.x uses floating point registers for optimization. This may
// occur anywhere in the code, even in IRQ and FIQ handlers.

#if RASPPI >= 2 && __GNUC__ >= 12

#ifndef SAVE_VFP_REGS_ON_IRQ
#define SAVE_VFP_REGS_ON_IRQ
#endif

#ifndef SAVE_VFP_REGS_ON_FIQ
#define SAVE_VFP_REGS_ON_FIQ
#endif

// save all VFP regs in exceptionstub.S
#ifndef __FAST_MATH__
#define __FAST_MATH__
#endif

#endif


// Sets the name of the "main()" entry point function that will be
// called by circle after system initialization has completed.
//
// 	extern int MAINPROC (void);
//
// Can be used by wrapper libraries that need to inject their
// own startup/shutdown code before calling their client's main().

#ifndef MAINPROC
#define MAINPROC main
#endif

///////////////////////////////////////////////////////////////////////

#include <circle/memorymap.h>

#endif
