Circle
======

> This is Step 2 of Circle. To get access to Step 1 use the git tag "Step1".

> If you read this file in an editor you should switch line wrapping on.

Overview
--------

Circle is a C++ bare metal environment for the Raspberry Pi. Currently it should run on all existing models but I can only test it on model B and B+. Later these will be the only supported because of the required on-board USB hub.

Circle will be developed and released step by step. So it may be easier to understand it. At the beginning the functionality is very limited. The main goal is to setup the classes needed for the USB stack now.

The 2nd Step
------------

The frame buffer is initialized (normally with maximum possible size) and a rectangle with a cross inside is drawn on it. Again the *Act LED* is used to demonstrate the performance boost by using the MMU. Watch the blink frequency without and with the MMU. The MMU can be enabled in the constructor of the class CKernel (see sample/kernel.cpp). You can create a file *cmdline.txt* like this on the SD(HC) card to change the frame buffer size:

`width=640 height=480`

Circle has the following new features:

* Set pixel on screen
* Use kernel options
* Switch on MMU

In Step 1 the following features were introduced:

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

The following C++ classes were added to the Circle library in the lib/ subdirectory:

* CBcmFrameBuffer: Frame buffer initialization, setting color palette for 8 bit depth.
* CKernelOptions: Providing kernel options from file cmdline.txt ("width=" and "height=").
* CMemorySystem: Enabling MMU if requested, switching page tables (not used here).
* CPageTable: Encapsulates a page table to be used by MMU.
* CScreenDevice: Manipulating the frame buffer contents (only setting pixels for now).

In Step 1 the following classes were introduced:

* CActLED: Switch the Act LED on and off, checks the Raspberry Pi model to use the right LED pin.
* CBcmMailBox: Simple GPU mailbox interface, currently used for the property interface.
* CBcmPropertyTags: Get several information from the GPU side or control something on this side.
* CGPIOPin: Encapsulates a GPIO pin, can be read, write or inverted. Simple initialization.
* CMemorySystem: Currently only initilizes the heap to the right memory size. Will be extended.
* CTimer: Currently only two simple static delay functions which work without creating an instance.
