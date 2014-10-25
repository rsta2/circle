Circle
======

> If you read this file in an editor you should switch line wrapping on.

Overview
--------

Circle is a C++ bare metal environment for the Raspberry Pi. Currently it should run on all existing models but I can only test it on model B and B+. Later these will be the only supported because of the required on-board USB hub.

Circle will be developed and released step by step. So it may be easier to understand it. At the beginning the functionality is very limited. The main goal is to setup the development environment first.

The First Step
--------------

The Act LED flashes 10 times and on the 3.5mm headphone jack 10 "clicks" are generated at the same time. Then the system reboots.

For now Circle has the following features:

* C++ build environment
* Simple delay functionality
* Get properties (model, memory size) from VideoCore
* new and delete
* Using GPIO pins
* Manipulating Act LED

Building
--------

Building is tested on PC Linux only. You need a [toolchain](http://elinux.org/Rpi_Software#ARM) for the ARM1176JZF core. First edit the file *Rules.mk* and set the PREFIX of your toolchain commands. Then go to the build root of Circle and do:

`./makeall clean`  
`./makeall`

The ready build *kernel.img* file should be in the sample/ directory.

You can also build Circle on the Raspberry Pi itself on Debian wheezy but you need some method to put the *kernel.img* file onto the SD(HC) card.

Installation
------------

Copy the Raspberry Pi firmware (from boot/ directory, do *make* there to get them) files along with the kernel.img (from sample/ directory) to a SD(HC) card with FAT file system. Put the SD(HC) card into the Raspberry Pi.

Directories
-----------

* include: The common header files, most class headers are in the include/circle/ subdirectory.
* lib: The Circle class implementation and support files.
* sample: A sample application using Circle. The main function is implemented in the CKernel class.
* boot: Do *make* in this directory to get the Raspberry Pi firmware files required to boot.

Classes
-------

The following C++ classes build currently the Circle library in the lib/ subdirectory:

* CActLED: Switch the Act LED on and off, checks the Raspberry Pi model to use the right LED pin.
* CBcmMailBox: Simple GPU mailbox interface, currently used for the property interface.
* CBcmPropertyTags: Get several information from the GPU side or control something on this side.
* CGPIOPin: Encapsulates a GPIO pin, can be read, write or inverted. Simple initialization.
* CMemorySystem: Currently only initilizes the heap to the right memory size. Will be extended.
* CTimer: Currently only two simple static delay functions which work without creating an instance.
