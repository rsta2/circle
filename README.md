Circle
======

> If you read this file in an editor you should switch line wrapping on.

Overview
--------

Circle is a C++ bare metal programming environment for the Raspberry Pi. It should be usable on all existing models before the Raspberry Pi 4 (tested on model A+, B, B+, on Raspberry Pi 2 and 3 and on Raspberry Pi Zero). It provides several ready-tested C++ classes which can be used to control different hardware features of the Raspberry Pi. Together with Circle there are delivered some samples which demonstrate the use of its classes. Circle can be used to create 32-bit or 64-bit bare metal applications.

Circle includes bigger (optional) third-party C-libraries for specific purposes in addon/ now. This is the reason why GitHub rates the project as a C-language-project. The main Circle libraries are written in C++ using classes instead. That's why it is named a C++ programming environment.

Release 39.2
------------

This is another intermediate release, which collects the recent changes to the project as a basis for the planned Raspberry Pi 4 support in Circle 40.

News in this release are:

* LittlevGL embedded GUI library (by Gabor Kiss-Vamosi) supported (in addon/littlevgl/).

* The SD card access (EMMC) performance has been remarkable improved.

* USB mass-storage devices (e.g. flash drives) can be removed from the running system now. The application has to call `CDevice::RemoveDevice()` for the device object of the USB mass-storage device, which will be removed afterwards. The file system has to be unmounted before by the application. If FatFs (in addon/fatfs/) is used, the device removal is announced by calling `disk_ioctl (pdrv, CTRL_EJECT, 0)`, where pdrv is the physical drive number (e.g. 1 for the first USB mass-storage device).

* Network initialization can be done in background now to speed-up system initialization. If `CNetSubSystem::Initialize()` is called with the parameter FALSE, it returns quickly without waiting for the Ethernet link to come up and for DHCP to be bound. The network must not be accessed, before `CNetSubSystem::IsRunning()` returns TRUE. This has to be ensured by the application.

Please note that the rudimentary Bluetooth support is deprecated now. There are legal reasons, why it cannot be developed further and because it is currently of very limited use, it will probably be removed soon.

The 39th Step
-------------

Circle supports the following accelerated graphics APIs now:

* OpenGL ES 1.1 and 2.0
* OpenVG 1.1
* EGL 1.4
* Dispmanx

This has been realized by (partially) porting the Raspberry Pi [userland libraries](https://github.com/raspberrypi/userland), which use the VC4 GPU to render the graphics. Please see the *addon/vc4/interface/* directory and the *README* file in this directory for more details. This support is limited to AArch32 and cannot be built on Raspbian.

The accelerated graphics support requires support for <math.h> functions. To provide this, the *libm.a* standard library is linked now, in case `STDLIB_SUPPORT = 1` is set (default). You need an appropriate toolchain so that it works. See the *Building* section for a link. You may use the <math.h> functions in your own applications too now.

Circle does not support normal USB hot-plugging, but there is a new feature, which allows to detect newly attached USB devices on application request. You can call CDWHCIDevice::ReScanDevices() now, while the application is running, to accomplish this.

The options to be used for *cmdline.txt* are described in *doc/cmdline.txt*.

Features
--------

Circle supports the following features:

| Group                 | Features                                            |
|-----------------------|-----------------------------------------------------|
| C++ build environment | AArch32 and AArch64 support                         |
|                       | Basic library functions (e.g. new and delete)       |
|                       | Enables all CPU caches using the MMU                |
|                       | Interrupt support (IRQ and FIQ)                     |
|                       | Multi-core support (Raspberry Pi 2 and 3)           |
|                       | Cooperative non-preemtive scheduler                 |
|                       | CPU clock rate management                           |
|                       |                                                     |
| Debug support         | Kernel logging to screen, UART and/or syslog server |
|                       | C-assertions with stack trace                       |
|                       | Hardware exception handler with stack trace         |
|                       | GDB support using rpi_stub (Raspberry Pi 2 and 3)   |
|                       | Serial bootloader (by David Welch) included         |
|                       | QEMU support (tested with AArch32 only)             |
|                       |                                                     |
| Legacy devices        | GPIO pins (with interrupt, Act LED) and clocks      |
|                       | Frame buffer (screen driver with escape sequences)  |
|                       | UART (Polling and interrupt driver)                 |
|                       | System timer (with kernel timers)                   |
|                       | Platform DMA controller                             |
|                       | EMMC SD card interface driver                       |
|                       | PWM output (2 channels)                             |
|                       | PWM sound output (on headphone jack)                |
|                       | I2C master and slave                                |
|                       | SPI0 master (Polling and DMA driver)                |
|                       | SPI1 auxiliary master (Polling)                     |
|                       | I2S sound output                                    |
|                       | Hardware random number generator                    |
|                       | Official Raspberry Pi touch screen                  |
|                       | VCHIQ interface and audio service drivers           |
|                       |                                                     |
| USB                   | Host controller interface (HCI) driver              |
|                       | Standard hub driver                                 |
|                       | HID class device drivers (keyboard, mouse, gamepad) |
|                       | Driver for on-board Ethernet device (SMSC951x)      |
|                       | Driver for on-board Ethernet device (LAN7800)       |
|                       | Driver for USB mass storage devices (bulk only)     |
|                       | Audio class MIDI input support                      |
|                       | Printer driver                                      |
|                       |                                                     |
| File systems          | Internal FAT driver (reduced function)              |
|                       | FatFs driver (full function, by ChaN)               |
|                       |                                                     |
| TCP/IP networking     | Protocols: ARP, IP, ICMP, UDP, TCP                  |
|                       | Clients: DHCP, DNS, NTP, HTTP, Syslog, MQTT         |
|                       | Servers: HTTP, TFTP                                 |
|                       | BSD-like C++ socket API                             |
|                       |                                                     |
| Graphics              | OpenGL ES 1.1 and 2.0, OpenVG 1.1, EGL 1.4          |
|                       | uGUI (by Achim Doebler)                             |
|                       | LittlevGL GUI library (by Gabor Kiss-Vamosi)        |
|                       |                                                     |
| Bluetooth             | Device inquiry support only                         |
| (deprecated)          | USB BR/EDR dongle driver                            |
|                       | Internal controller of Raspberry Pi 3 B             |

Building
--------

> For building 64-bit applications (AArch64) see the next section.

Building is normally done on PC Linux. If building for the Raspberry Pi 1 you need a [toolchain](http://elinux.org/Rpi_Software#ARM) for the ARM1176JZF core (with EABI support). For Raspberry Pi 2/3 you need a toolchain with Cortex-A7/-A53 support. A toolchain, which works for all of these, can be downloaded [here](https://developer.arm.com/open-source/gnu-toolchain/gnu-a/downloads). Circle has been tested with the version *8.2-2019.01* (gcc-arm-8.2-2019.01-x86_64-arm-eabi.tar.xz) from this website.

First edit the file *Rules.mk* and set the Raspberry Pi version (*RASPPI*, 1, 2 or 3) and the *PREFIX* of your toolchain commands. Alternatively you can create a *Config.mk* file (which is ignored by git) and set the Raspberry Pi version and the *PREFIX* variable to the prefix of your compiler like this (don't forget the dash at the end):

`RASPPI = 1`  
`PREFIX = arm-none-eabi-`

The following table gives support for selecting the right *RASPPI* value:

| RASPPI | Target         | Models                   | Optimized for |
| ------ | -------------- | ------------------------ | ------------- |
|      1 | kernel.img     | A, B, A+, B+, Zero, (CM) | ARM1176JZF-S  |
|      2 | kernel7.img    | 2, 3, (CM3)              | ARMv7-A       |
|      3 | kernel8-32.img | 3, (CM3)                 | Cortex-A53    |

For a binary distribution you should do one build with *RASPPI = 1* and one with *RASPPI = 2* and include the created files *kernel.img* and *kernel7.img*. Optionally you can do a build with *RASPPI = 3* and add the created file *kernel8-32.img* to provide an optimized version for the Raspberry Pi 3.

Then go to the build root of Circle and do:

`./makeall clean`  
`./makeall`

By default only the latest sample (with the highest number) is build. The ready build *kernel.img* file should be in its subdirectory of sample/. If you want to build another sample after `makeall` go to its subdirectory and do `make`.

You can also build Circle on the Raspberry Pi itself on Raspbian but you need some method to put the *kernel.img* file onto the SD(HC) card. With an external USB card reader on model B+ or Raspberry Pi 2/3 model B (4 USB ports) this should be no problem.

Building Circle from a non-Linux host is possible too. Maybe you have to adapt the shell scripts in this case. You need a cross compiler targetting (for example) *arm-none-eabi*. OSDev.org has an [excellent document on the subject](http://wiki.osdev.org/GCC_Cross-Compiler) that you can follow if you have no idea of what a cross compiler is, or how to make one.

AArch64
-------

Circle supports building 64-bit applications, which can be run on the Raspberry Pi 3. There are also Raspberry Pi 2 versions, which are based on the BCM2837 SoC. These Raspberry Pi versions can be used too.

The recommended toolchain to build 64-bit applications with Circle can be downloaded [here](https://developer.arm.com/open-source/gnu-toolchain/gnu-a/downloads). Circle has been tested with the version *8.2-2019.01* (gcc-arm-8.2-2019.01-x86_64-aarch64-elf.tar.xz) from this website.

First edit the file *Rules.mk* and set the Raspberry Pi architecture (*AARCH*, 32 or 64) and the *PREFIX64* of your toolchain commands. The *RASPPI* variable is set automatically to 3 for `AARCH = 64` and does not need to be set here. Alternatively you can create a *Config.mk* file (which is ignored by git) and set the Raspberry Pi architecture and the *PREFIX64* variable to the prefix of your compiler like this (don't forget the dash at the end):

```
AARCH = 64
PREFIX64 = aarch64-elf-
```

Then go to the build root of Circle and do:

```
./makeall clean
./makeall
```

By default only the latest sample (with the highest number) is build. The ready build *kernel8.img* file should be in its subdirectory of sample/. If you want to build another sample after `makeall` go to its subdirectory and do `make`.

Installation
------------

Copy the Raspberry Pi firmware (from boot/ directory, do *make* there to get them) files along with the kernel.img (from sample/ subdirectory) to a SD(HC) card with FAT file system. Put the SD(HC) card into the Raspberry Pi.

Directories
-----------

* include: The common header files, most class headers are in the include/circle/ subdirectory.
* lib: The Circle class implementation and support files (other libraries are in subdirectories of lib/).
* sample: Several sample applications using Circle in different subdirectories. The main function is implemented in the CKernel class.
* addon: Contains contributed libraries and samples (has to be build manually).
* app: Place your own applications here. If you have own libraries put them into app/lib/.
* boot: Do *make* in this directory to get the Raspberry Pi firmware files required to boot.
* doc: Additional documentation files.
* tools: Some tools for using Circle more comfortable (e.g. a serial bootloader).

Classes
-------

The following C++ classes were added to Circle:

Base library

* CBcm54213Device: Driver for BCM54213PE Gigabit Ethernet Transceiver of Raspberry Pi 4.
* CBcmPCIeHostBridge: Driver for PCIe Host Bridge of Raspberry Pi 4.

USB library

* CUSBHCIDevice: Alias for CDWHCIDevice or CXHCIDevice, depending on Raspberry Pi model.
* CUSBHCIRootPort: Base class, which represents an USB HCI root port.
* CXHCIDevice: USB host controller interface (HCI) driver for Raspberry Pi 4 (incomplete).

Net library

* CPHYTask: Background task which continuously updates the PHY of the used net device.

The available Circle classes are listed in the file *doc/classes.txt*. If you have doxygen installed on your computer you can build a class documentation in *doc/html/* using:

`./makedoc`

At the moment there are only a few classes described in detail for doxygen.

Trademarks
----------

Raspberry Pi is a trademark of the Raspberry Pi Foundation.

Linux is a trademark of Linus Torvalds.

PS3 and PS4 are registered trademarks of Sony Computer Entertainment Inc.

Xbox 360 and Xbox One are trademarks of the Microsoft group of companies.

Nintendo Switch is a trademark of Nintendo.

Khronos and OpenVG are trademarks of The Khronos Group Inc.

OpenGL ES is a trademark of Silicon Graphics Inc.
