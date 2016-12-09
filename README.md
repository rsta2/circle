Circle
======

> Raspberry Pi is a trademark of the Raspberry Pi Foundation.

> This is Step 27 of Circle. To get access to Step 1-26 use the git tag "Step1" to "Step26".

> If you read this file in an editor you should switch line wrapping on.

Overview
--------

Circle is a C++ bare metal programming environment for the Raspberry Pi. It should be useable on all existing models (tested on model A+, B, B+, on Raspberry Pi 2 and 3 and on Raspberry Pi Zero). It provides several ready-tested C++ classes which can be used to control different hardware features of the Raspberry Pi. Together with Circle there are delivered some samples which demonstrate the use of its classes.

Please note that the included USB library was developed in a hobby project. There are known issues with it (e.g. no dynamic attachments, no error recovery, limited split support). For me it works well but that need not be the case with any device and in any situation.

A Real-Time OS?
---------------

Circle is not a real-time OS. That means different simultaneous operations may interfere in respect of its timing behaviour. The provided samples are tested to work but if you try different combinations of hardware support classes this has to be tested by yourself.

Nevertheless real-time applications based on Circle are possible. Have a look at *doc/realtime.txt* for more information!

The 27th Step
-------------

This step adds some basic support for USB gamepads with a standard USB HID class report interface (3-0-0). It is demonstrated in a simple test program in *sample/27-usbgamepad*. See the *README* file in this directory for details. The USB HID class gamepad driver was ported from USPi and was originally developed by Marco Maccaferri. Please note that the gamepad API may change in the future if further gamepads will be supported.

The USB keyboard driver has three new keyboard maps now (FR, IT and US International) and can handle the keyboard status LEDs, if the application supports this, like shown in *sample/08-usbkeyboard*. The USB mouse driver has been extended to support a mouse cursor. *sample/10-usbmouse* has been updated to use this.

TFTP file server support has been added to addon/ which is intended to support kernel image and firmware updates while developing and testing.

The Bluetooth library is working with the internal Bluetooth host controller of the Raspberry Pi 3 now. The support is still limited to the Bluetooth inquiry function (see *sample/22-btsimple*).

There is an important change in the networking code. UDP sockets do not send and receive broadcast messages by default any more for security reasons. CSocket::SetOptionBroadcast(TRUE) has to be called after Bind() or Connect() to allow this. So if you use broadcasts with UDP sockets you have to update your application.

The options to be used for *cmdline.txt* are described in *doc/cmdline.txt*.

In Step 1-26 the following features were introduced:

* C++ build environment
* Simple delay functionality
* Get properties (model, memory size) from VideoCore
* new and delete
* Using GPIO pins
* Manipulating Act LED
* Set pixel on screen
* Use kernel options
* Switch on MMU
* Formatting strings
* Using devices
* Writing characters to screen
* Writing characters to UART
* Logging output to screen or UART
* Using assertions and debug hexdump
* Using interrupts
* Timer class with clock, timers and calibrated delay loop
* Exception handler
* USB host controller interface (HCI) driver (no split support for now)
* USB device class (basic device initialization)
* USB hub driver (dectects and enables the supported devices)
* Driver for the on-board Ethernet device (receiving and transmitting frames)
* Driver for USB mass storage devices (bulk only, read and write)
* Detects low- and full-speed devices
* Driver for USB keyboards
* Using GPIO interrupts
* Driver for USB mice
* Using GPIO clock
* Simple 12 MHz GPIO sampling routine
* PWM sound device
* DMA controller support
* PWM output (2 channels)
* Simple USB printer support
* FAT file system support (reduced)
* I2C (master and slave) support
* Multi-core support on Raspberry Pi 2
* Experimental (UDP only) TCP/IP network stack
* Setting system time from an Internet time (NTP) server
* Cooperative non-preemtive scheduler
* TCP support
* DHCP support
* Simple HTTP webserver class
* GDB debug support on Raspberry Pi 2 using rpi_stub
* Bluetooth device inquiry support
* SPI0 master support
* Driver for hardware random number generator
* SPI0 master support using DMA
* CPU clock rate management support

Building
--------

Building is normally done on PC Linux. If building for the Raspberry Pi 1 you need a [toolchain](http://elinux.org/Rpi_Software#ARM) for the ARM1176JZF core (with EABI support). For Raspberry Pi 2/3 you need a toolchain with Cortex-A7/-A53 support. [This one](https://github.com/raspberrypi/tools/tree/master/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf) should work for all of these.

First edit the file *Rules.mk* and set the Raspberry Pi version (*RASPPI*, 1, 2 or 3) and the *PREFIX* of your toolchain commands. Alternatively you can create a *Config.mk* file (which is ignored by git) and set the Raspberry Pi version and the *PREFIX* variable to the prefix of your compiler like this (don't forget the dash at the end):

`RASPPI = 1`  
`PREFIX = arm-none-eabi-`

The following table gives support for selecting the right *RASPPI* value:

| RASPPI | Target      | Models                   | Optimized for |
| ------ | ----------- | ------------------------ | ------------- |
|      1 | kernel.img  | A, B, A+, B+, Zero, (CM) | ARM1176JZF-S  |
|      2 | kernel7.img | 2, 3                     | ARMv7-A       |
|      3 | kernel7.img | 3                        | Cortex-A53    |

For a binary distribution you should do one build with *RASPPI = 1* and one with *RASPPI = 2* and include the created files *kernel.img* and *kernel7.img*.

Then go to the build root of Circle and do:

`./makeall clean`  
`./makeall`

By default only the latest sample (with the highest number) is build. The ready build *kernel.img* file should be in its subdirectory of sample/. If you want to build another sample after `makeall` go to its subdirectory and do `make`.

You can also build Circle on the Raspberry Pi itself on Raspbian but you need some method to put the *kernel.img* file onto the SD(HC) card. With an external USB card reader on model B+ or Raspberry Pi 2/3 model B (4 USB ports) this should be no problem.

Building Circle from a non-Linux host is possible too. Maybe you have to adapt the shell scripts in this case. You need a cross compiler targetting (for example) *arm-none-eabi*. OSDev.org has an [excellent document on the subject](http://wiki.osdev.org/GCC_Cross-Compiler) that you can follow if you have no idea of what a cross compiler is, or how to make one.

Installation
------------

Copy the Raspberry Pi firmware (from boot/ directory, do *make* there to get them) files along with the kernel.img (from sample/ subdirectory) to a SD(HC) card with FAT file system. Put the SD(HC) card into the Raspberry Pi.

Note that the file *kernel.img* has been renamed to *kernel7.img* for the Raspberry Pi 2/3.

Directories
-----------

* include: The common header files, most class headers are in the include/circle/ subdirectory.
* lib: The Circle class implementation and support files (other libraries are in subdirectories of lib/).
* sample: Several sample applications using Circle in different subdirectories. The main function is implemented in the CKernel class.
* addon: Contains contributed libraries and samples (has to be build manually).
* app: Place your own applications here. If you have own libraries put them into app/lib/.
* boot: Do *make* in this directory to get the Raspberry Pi firmware files required to boot.
* doc: Additional documentation files.

Classes
-------

The following C++ classes were added to Circle:

USB library

* CUSBGamePadDevice: Driver for USB gamepads with USB HID class report interface (3-0-0)

Input library

* CMouseBehaviour: Generic mouse function, handles the mouse cursor

Net library

* CRouteCache: Caches special routes, received via ICMP redirect requests
* CTFTPDaemon: TFTP server task

Bluetooth library

* CBTUARTTransport: Bluetooth UART HCI transport driver for the internal BT device of the Raspberry Pi 3

The available Circle classes are listed in the file *doc/classes.txt*. If you have doxygen installed on your computer you can build a class documentation in *doc/html/* using:

`./makedoc`

At the moment there are only a few classes described in detail for doxygen.
