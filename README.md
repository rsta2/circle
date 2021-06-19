Circle
======

> If you read this file in an editor you should switch line wrapping on.

Overview
--------

Circle is a C++ bare metal programming environment for the Raspberry Pi. It should be usable on all existing models (tested on model A+, B, B+, on Raspberry Pi 2, 3, 4, 400 and on Raspberry Pi Zero), except on the Raspberry Pi Pico, which is not supported. Circle provides several ready-tested [C++ classes](doc/classes.txt) and [add-on libraries](addon/README), which can be used to control different hardware features of the Raspberry Pi. Together with Circle there are delivered several [sample programs](sample/README), which demonstrate the use of its classes. Circle can be used to create 32-bit or 64-bit bare metal applications.

Circle includes bigger (optional) third-party C-libraries for specific purposes in addon/ now. This is the reason why GitHub rates the project as a C-language-project. The main Circle libraries are written in C++ using classes instead. That's why it is named a C++ programming environment.

Release 44.1
------------

This hotfix release solves the following issues:

* Do not use platform DMA12 controller, which has no dedicated IRQ line (Raspberry Pi 1-3 and Zero).
* M/S mode in class CPWMOutput did not work on channel 2.
* memmove() did not work together with circle-stdlib project. Now always implemented in Circle itself.
* doc/qemu.txt updated to refer to patched QEMU v6.0.0 to be used with Circle.
* doc/stdlib-support.txt referred to removed build.bash script.

The 44th Step
-------------

This release comes with new features, improvements and bug fixes. There is a new HDMI sound driver class `CHDMISoundBaseDevice`, which allows to generate **HDMI sound without VCHIQ** driver, which can be easier to integrate in an application. This is shown by the [sample/29-miniorgan](sample/29-miniorgan) and [sample/34-sounddevices](sample/34-sounddevices). On the Raspberry Pi 4 only the connector HDMI0 is supported. The class `CI2SSoundBaseDevice` now supports the **PCM5122 DAC**.

A new class ``C2DGraphics`` has been added to the base library, which provides **2D drawing routines**, which work without flickering or screen tearing. This is demonstrated in the [sample/41-screenanimations](sample/41-screenanimations).

The **scheduler library** has been improved and provides the new classes `CMutex` and `CSemaphore`. Multiple tasks can wait for a `CSynchronzationEvent` to be set now.

There is a **new serial bootloader and flash tool** (Flashy), which improves the download speed and reliability. Please see the second part of the file [doc/bootloader.txt](doc/bootloader.txt) for more information! You can interrupt the download process with Ctrl-C now and start again, without resetting your Raspberry Pi. You should update your bootloader kernel image(s) on the SD card in any case. The old flash tool is still available.

Circle comes with a **configure script** now, which can be used to create the configuration file `Config.mk` easier. Please enter `configure -h` for a description of its options.

The C++ support has been improved. Now **placement new operators** and **static objects inside of a function** can be used. Furthermore the **C++17 standard** is optionally supported and can be enabled with the option `--c++17` of `configure`, if you have a toolchain version, which supports it.

Further improvements:

* There is a new system option `NO_BUSY_WAIT`. With this option enabled, the EMMC, SDHOST and USB drivers will **not busy wait for the completion of synchronous transfers** any more. This should improve system throughput and network latency, but requires the scheduler in the system.
* The **embedded MMC memory of the Compute Module 4** can be accessed, when the system option `USE_EMBEDDED_MMC_CM4` has been defined.
* The class `CTFTPFatFsFileServer` was added to [addon/tftpfileserver](addon/tftpfileserver) to support **TFTP access with the FatFs filesystem module**.
* The class `CDS18x20` in [addon/OneWire](addon/OneWire) has been improved and is now part of the library, not of the sample as before. It determines the used power mode of the sensor automatically.
* Functions for **atomic memory access** have been added to `<circle/atomic.h>`.

Bug fixes:

* System timer IRQ handling may have stopped working after a while on the Raspberry Pi 1 and Zero before.
* xHCI USB controller did not work on some Raspberry Pi 4 models.
* Starting secondary cores 1-3 was not reliable.
* Access to USB mass-storage devices was not reliable on Raspberry Pi Model A+, 3A+ and Zero before.
* Add workaround for non-compliant low-speed USB devices with bulk endpoints.
* Suppress concurrent split IN/OUT requests on Raspberry Pi 1-3 and Zero in USB serial drivers.
* Enable serial FIFO in polling mode too.
* The screen size select-able in *cmdline.txt* was limited to 1920x1080 before.
* Semaphore implementation in *addon/linux* was not IRQ safe, but used from IRQ handler in VCHIQ driver.
* Allow received text segment in TCP state SYN-RECEIVED.

Don't forget to update the used firmware to the one downloadable in [boot/](boot/)!

Features
--------

Circle supports the following features:

| Group                 | Features                                            |
|-----------------------|-----------------------------------------------------|
| C++ build environment | AArch32 and AArch64 support                         |
|                       | Basic library functions (e.g. new and delete)       |
|                       | Enables all CPU caches using the MMU                |
|                       | Interrupt support (IRQ and FIQ)                     |
|                       | Multi-core support (Raspberry Pi 2, 3 and 4)        |
|                       | Cooperative non-preemtive scheduler                 |
|                       | CPU clock rate management                           |
|                       |                                                     |
| Debug support         | Kernel logging to screen, UART and/or syslog server |
|                       | C-assertions with stack trace                       |
|                       | Hardware exception handler with stack trace         |
|                       | GDB support using rpi_stub (Raspberry Pi 2 and 3)   |
|                       | Serial bootloader (by David Welch) included         |
|                       | Software profiling support (single-core)            |
|                       | QEMU support                                        |
|                       |                                                     |
| SoC devices           | GPIO pins (with interrupt, Act LED) and clocks      |
|                       | Frame buffer (screen driver with escape sequences)  |
|                       | UART(s) (Polling and interrupt driver)              |
|                       | System timer (with kernel timers)                   |
|                       | Platform DMA controller                             |
|                       | EMMC SD card interface driver                       |
|                       | SDHOST SD card interface driver (Raspberry Pi 1-3)  |
|                       | PWM output (2 channels)                             |
|                       | PWM sound output (on headphone jack)                |
|                       | I2C master(s) and slave                             |
|                       | SPI0 master (Polling and DMA driver)                |
|                       | SPI1 auxiliary master (Polling)                     |
|                       | SPI3-6 masters of Raspberry Pi 4 (Polling)          |
|                       | I2S sound output                                    |
|                       | HDMI sound output (without VCHIQ)                   |
|                       | Hardware random number generator                    |
|                       | Official Raspberry Pi touch screen                  |
|                       | VCHIQ interface and audio service drivers           |
|                       | BCM54213PE Gigabit Ethernet NIC of Raspberry Pi 4   |
|                       | Wireless LAN access (experimental)                  |
|                       |                                                     |
| USB                   | Host controller interface (HCI) drivers             |
|                       | Standard hub driver (USB 2.0 only)                  |
|                       | HID class device drivers (keyboard, mouse, gamepad) |
|                       | Driver for on-board Ethernet device (SMSC951x)      |
|                       | Driver for on-board Ethernet device (LAN7800)       |
|                       | Driver for USB mass storage devices (bulk only)     |
|                       | Drivers for different USB serial devices            |
|                       | Audio class MIDI input support                      |
|                       | Printer driver                                      |
|                       |                                                     |
| File systems          | Internal FAT driver (limited function)              |
|                       | FatFs driver (full function, by ChaN)               |
|                       |                                                     |
| TCP/IP networking     | Protocols: ARP, IP, ICMP, UDP, TCP                  |
|                       | Clients: DHCP, DNS, NTP, HTTP, Syslog, MQTT         |
|                       | Servers: HTTP, TFTP                                 |
|                       | BSD-like C++ socket API                             |
|                       |                                                     |
| Graphics              | OpenGL ES 1.1 and 2.0, OpenVG 1.1, EGL 1.4          |
|                       | (not on Raspberry Pi 4)                             |
|                       | uGUI (by Achim Doebler)                             |
|                       | LVGL (by LVGL LLC)                                  |
|                       | 2D graphics class in base library                   |
|                       |                                                     |
| Not supported         | Bluetooth                                           |
|                       | Camera                                              |
|                       | USB device (gadget) mode                            |
|                       | USB isochronous transfers and audio                 |

Building
--------

> For building 64-bit applications (AArch64) see the next section.

This describes building on PC Linux. See the file [doc/windows-build.txt](doc/windows-build.txt) for information about building on Windows. If building for the Raspberry Pi 1 you need a [toolchain](http://elinux.org/Rpi_Software#ARM) for the ARM1176JZF core (with EABI support). For Raspberry Pi 2/3/4 you need a toolchain with Cortex-A7/-A53/-A72 support. A toolchain, which works for all of these, can be downloaded [here](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads). Circle has been tested with the version *10.2-2020.11* (gcc-arm-10.2-2020.11-x86_64-arm-none-eabi.tar.xz) from this website.

First edit the file *Rules.mk* and set the Raspberry Pi version (*RASPPI*, 1, 2, 3 or 4) and the *PREFIX* of your toolchain commands. Alternatively you can create a *Config.mk* file (which is ignored by git) and set the Raspberry Pi version and the *PREFIX* variable to the prefix of your compiler like this (don't forget the dash at the end):

```
RASPPI = 1
PREFIX = arm-none-eabi-
```

The following table gives support for selecting the right *RASPPI* value:

| RASPPI | Target         | Models                   | Optimized for |
| ------ | -------------- | ------------------------ | ------------- |
|      1 | kernel.img     | A, B, A+, B+, Zero, (CM) | ARM1176JZF-S  |
|      2 | kernel7.img    | 2, 3, (CM3)              | Cortex-A7     |
|      3 | kernel8-32.img | 3, (CM3)                 | Cortex-A53    |
|      4 | kernel7l.img   | 4B, 400, CM4             | Cortex-A72    |

For a binary distribution you should do one build with *RASPPI = 1*, one with *RASPPI = 2* and one build with *RASPPI = 4* and include the created files *kernel.img*, *kernel7.img* and *kernel7l.img*. Optionally you can do a build with *RASPPI = 3* and add the created file *kernel8-32.img* to provide an optimized version for the Raspberry Pi 3.

The configuration file *Config.mk* can be created using the `configure` tool too. Please enter `./configure -h` for help on using it!

Then go to the build root of Circle and do:

```
./makeall clean
./makeall
```

By default only the latest sample (with the highest number) is build. The ready build *kernel.img* file should be in its subdirectory of sample/. If you want to build another sample after `makeall` go to its subdirectory and do `make`.

You can also build Circle on the Raspberry Pi itself (set `PREFIX =` (empty)) on Raspbian but you need some method to put the *kernel.img* file onto the SD(HC) card. With an external USB card reader on model B+ or Raspberry Pi 2/3/4 model B (4 USB ports) this should be no problem.

AArch64
-------

Circle supports building 64-bit applications, which can be run on the Raspberry Pi 3 or 4. There are also Raspberry Pi 2 versions, which are based on the BCM2837 SoC. These Raspberry Pi versions can be used too.

The recommended toolchain to build 64-bit applications with Circle can be downloaded [here](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads). Circle has been tested with the version *10.2-2020.11* (gcc-arm-10.2-2020.11-x86_64-aarch64-none-elf.tar.xz) from this website.

There are distro-provided toolchains on certain Linux platforms (e.g. *g++-aarch64-linux-gnu* on Ubuntu or *gcc-c++-aarch64-linux-gnu* on Fedora), which may work with Circle and can be a quick way to use it, but you have to test this by yourself. If you encounter problems (e.g. no reaction at all, link failure with external library) using a distro-provided toolchain, please try the recommended toolchain (see above) first, before reporting an issue.

First edit the file *Rules.mk* and set the Raspberry Pi architecture (*AARCH*, 32 or 64) and the *PREFIX64* of your toolchain commands. The *RASPPI* variable has to be set to 3 or 4 for `AARCH = 64`. Alternatively you can create a *Config.mk* file (which is ignored by git) and set the Raspberry Pi architecture and the *PREFIX64* variable to the prefix of your compiler like this (don't forget the dash at the end):

```
AARCH = 64
RASPPI = 3
PREFIX64 = aarch64-none-elf-
```

The configuration file *Config.mk* can be created using the `configure` tool too. Please enter `./configure -h` for help on using it!

Then go to the build root of Circle and do:

```
./makeall clean
./makeall
```

By default only the latest sample (with the highest number) is build. The ready build *kernel8.img* or *kernel8-rpi4.img* file should be in its subdirectory of sample/. If you want to build another sample after `makeall` go to its subdirectory and do `make`.

Installation
------------

Copy the Raspberry Pi firmware (from boot/ directory, do *make* there to get them) files along with the *kernel.img* (from sample/ subdirectory) to a SD(HC) card with FAT file system. Put the SD(HC) card into the Raspberry Pi.

The *config32.txt* file, provided in the boot/ directory, is needed to enable FIQ use in 32-bit mode on the Raspberry Pi 4 and has to be copied to the SD card in this case (rename it to config.txt). Furthermore the additional file *armstub7-rpi4.bin* is required on the SD card then. Please see [boot/README](boot/README) for information on how to build this file.

The *config64.txt* file, provided in the boot/ directory, is needed to enable 64-bit mode and has to be copied to the SD card in this case (rename it to config.txt). FIQ support for AArch64 on the Raspberry Pi 4 requires an additional file *armstub8-rpi4.bin* on the SD card. Please see [boot/README](boot/README) for information on how to build this file.

Directories
-----------

* include: The common header files, most class headers are in the include/circle/ subdirectory.
* lib: The Circle class implementation and support files (other libraries are in subdirectories of lib/).
* sample: Several sample applications using Circle in different subdirectories. The main function is implemented in the CKernel class.
* addon: Contains contributed libraries and samples (has to be build manually).
* app: Place your own applications here. If you have own libraries put them into app/lib/.
* boot: Do *make* in this directory to get the Raspberry Pi firmware files required to boot.
* doc: Additional documentation files.
* tools: Tools for building Circle and for using Circle more comfortable (e.g. a serial bootloader).

Classes
-------

The following C++ classes were added to Circle:

Base library

* C2DGraphics: Software graphics library with VSync and hardware-accelerated double buffering
* CGenericLock: Locks a resource with or without scheduler
* CHDMISoundBaseDevice: Low level access to the HDMI sound device (without VCHIQ)

Scheduler library

* CMutex: Provides a method to provide mutual exclusion (critical sections) across tasks
* CSemaphore: Implements a semaphore synchronization class

The available Circle classes are listed in the file [doc/classes.txt](doc/classes.txt). If you have Doxygen installed on your computer you can build a [class documentation](doc/html/index.html) in doc/html/ using:

`./makedoc`

At the moment there are only a few classes described in detail for Doxygen.

Additional Topics
-----------------

* [Standard library support](doc/stdlib-support.txt)
* [Dynamic memory management and the "new" operator](doc/new-operator.txt)
* [DMA buffer requirements](doc/dma-buffer-requirements.txt)
* [Serial bootloader support](doc/bootloader.txt)
* [Multi-core support](doc/multicore.txt)
* [USB plug-and-play](doc/usb-plug-and-play.txt)
* [Debugging support](doc/debug.txt)
* [JTAG debugging](doc/debug-jtag.txt)
* [QEMU support](doc/qemu.txt)
* [Eclipse IDE support](doc/eclipse-support.txt)
* [About real-time applications](doc/realtime.txt)
* [cmdline.txt options](doc/cmdline.txt)
* [Screen escape sequences](doc/screen.txt)
* [Keyboard escape sequences](doc/keyboard.txt)
* [Memory layout](doc/memorymap.txt)
* [Known issues](doc/issues.txt)

Trademarks
----------

Raspberry Pi is a trademark of the Raspberry Pi Foundation.

Linux is a trademark of Linus Torvalds.

PS3 and PS4 are registered trademarks of Sony Computer Entertainment Inc.

Windows, Xbox 360 and Xbox One are trademarks of the Microsoft group of companies.

Nintendo Switch is a trademark of Nintendo.

Khronos and OpenVG are trademarks of The Khronos Group Inc.

OpenGL ES is a trademark of Silicon Graphics Inc.

The micro:bit brand belongs to the Micro:bit Educational Foundation.

HDMI is a registered trademark of HDMI Licensing Administrator, Inc.
