Circle
======

> Raspberry Pi is a trademark of the Raspberry Pi Foundation.

> If you read this file in an editor you should switch line wrapping on.

Overview
--------

Circle is a C++ bare metal programming environment for the Raspberry Pi. It should be useable on all existing models (tested on model A+, B, B+, on Raspberry Pi 2 and 3 and on Raspberry Pi Zero). It provides several ready-tested C++ classes which can be used to control different hardware features of the Raspberry Pi. Together with Circle there are delivered some samples which demonstrate the use of its classes.

Circle includes bigger (optional) third-party C-libraries for specific purposes in addon/ now. This is the reason why GitHub rates the project as a C-language-project. The main Circle libraries are written in C++ using classes instead. That's why it is named a C++ programming environment.

Release 35.1
------------

This intermediate release is done to allow a new release of the [circle-stdlib project](https://github.com/smuehlst/circle-stdlib), which provides C and C++ standard library support for Circle and which has been extended with SSL/TLS support by porting [mbed TLS](https://tls.mbed.org/).

Another new feature in this release is a driver for LCD dot-matrix displays with HD44780 controller in the *addon/display/* subdirectory.

The 35th Step
-------------

In this step a client for the MQTT IoT protocol has been added to Circle. It is demonstrated in *sample/35-mqttclient*. See the *README* file in this directory for details.

Furthermore a serial bootloader is available now and can be started directly from the Circle build environment. The well-known bootloader by David Welch is used here. See the file *doc/eclipse-support.txt* for instructions on using the bootloader with or without the Eclipse IDE.

There is an incompatible change in behaviour of the CSocket API. Previously when a TCP connection received a FIN (disconnect) and the application called CSocket::Receive(), it did not get an error so far. Because of this, the disconnect has not been detectable. Now an error will be returned in this case. This may cause problems with existing user applications using TCP. You may have to check this.

The options to be used for *cmdline.txt* are described in *doc/cmdline.txt*.

Features
--------

Circle supports the following features:

| Group                 | Features                                            |
|-----------------------|-----------------------------------------------------|
| C++ build environment | Basic library functions (e.g. new and delete)       |
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
|                       | QEMU support                                        |
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
| GUI                   | uGUI (by Achim Doebler)                             |
|                       |                                                     |
| Bluetooth             | Device inquiry support only                         |
|                       | USB BR/EDR dongle driver                            |
|                       | Internal controller of Raspberry Pi 3 B             |

Building
--------

Building is normally done on PC Linux. If building for the Raspberry Pi 1 you need a [toolchain](http://elinux.org/Rpi_Software#ARM) for the ARM1176JZF core (with EABI support). For Raspberry Pi 2/3 you need a toolchain with Cortex-A7/-A53 support. A toolchain, which works for all of these, can be downloaded [here](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads). Circle has been tested with the version *6-2017-q1-update* from this website.

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

Net library

* CMQTTClient: Client for the MQTT IoT protocol
* CMQTTReceivePacket: MQTT helper class
* CMQTTSendPacket: MQTT helper class
* CNetSocket: Base class of networking sockets

The available Circle classes are listed in the file *doc/classes.txt*. If you have doxygen installed on your computer you can build a class documentation in *doc/html/* using:

`./makedoc`

At the moment there are only a few classes described in detail for doxygen.
