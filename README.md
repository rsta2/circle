Circle
======

> This is Step 11 of Circle. To get access to Step 1-10 use the git tag "Step1" to "Step10".

> If you read this file in an editor you should switch line wrapping on.

Overview
--------

Circle is a C++ bare metal programming environment for the Raspberry Pi. It should be useable on all existing models (tested on model A+, B and B+). It provides several ready-tested C++ classes which can be used to control different hardware features of the Raspberry Pi. Together with Circle there are delivered some samples which demonstrate the use of its classes.

Please note that the included USB library was developed in a hobby project. There are known issues with it (e.g. no dynamic attachments, no error recovery, limited split support). For me it works well but that need not be the case with any device and in any situation.

The 11th Step
-------------

In this step GPIO clock support is added and demonstrated in a simple "oscilloscope program" in *sample/11-gpioclock*. GPIO clock 0 generates a 125 KHz square wave signal which is sampled with about 12 MHz and displayed on the screen afterwards. See the *README* file in this directory for details.

The options to be used for *cmdline.txt* are described in *doc/cmdline.txt*.

In Step 1-10 the following features were introduced:

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

Building
--------

Building is normally done on PC Linux. You need a [toolchain](http://elinux.org/Rpi_Software#ARM) for the ARM1176JZF core. First edit the file *Rules.mk* and set the *PREFIX* of your toolchain commands. Alternatively you can create a *Config.mk* file (which is ignored by git) and set the *PREFIX* variable to the prefix of your compiler like this (don't forget the dash at the end):

`PREFIX = arm-none-eabi-`

Then go to the build root of Circle and do:

`./makeall clean`  
`./makeall`

By default only the latest sample (with the highest number) is build. The ready build *kernel.img* file should be in its subdirectory of sample/. If you want to build another sample after `makeall` go to its subdirectory and do `make`.

You can also build Circle on the Raspberry Pi itself on Debian wheezy but you need some method to put the *kernel.img* file onto the SD(HC) card. With an external USB card reader on model B+ (4 USB ports) this should be no problem.

Building Circle from a non-Linux host is possible too. Maybe you have to adapt the shell scripts in this case. You need a cross compiler targetting (for example) *arm-none-eabi*. OSDev.org has an [excellent document on the subject](http://wiki.osdev.org/GCC_Cross-Compiler) that you can follow if you have no idea of what a cross compiler is, or how to make one.

Installation
------------

Copy the Raspberry Pi firmware (from boot/ directory, do *make* there to get them) files along with the kernel.img (from sample/ subdirectory) to a SD(HC) card with FAT file system. Put the SD(HC) card into the Raspberry Pi.

Directories
-----------

* include: The common header files, most class headers are in the include/circle/ subdirectory.
* lib: The Circle class implementation and support files (other libraries are in subdirectories of lib/).
* sample: Several sample applications using Circle in different subdirectories. The main function is implemented in the CKernel class.
* addon: Contains contributed libraries and samples (only one at the moment, has to be build manually).
* boot: Do *make* in this directory to get the Raspberry Pi firmware files required to boot.
* doc: Additional documentation files.

Classes
-------

The following C++ classes were added to the Circle base library:

* CGPIOClock: Using GPIO clocks, initialize, start and stop it.

The available Circle classes are listed in the file *doc/classes.txt*.
