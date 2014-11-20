Circle
======

> This is Step 10 of Circle. To get access to Step 1-9 use the git tag "Step1" to "Step9".

> If you read this file in an editor you should switch line wrapping on.

Overview
--------

Circle is a C++ bare metal environment for the Raspberry Pi. The main library should be useable on all existing models (but tested on model B and B+ only) but the USB library is working only on model B and B+ for now.

There is a model-check in the USB library to prevent non-high-speed USB devices from overclocking because the USB physical layer is initialized to high-speed PHY. Furthermore there is an USB hub required to be connected to the root port to ensure device enumeration is working. This is always the case with model B and B+.

Please note that the included USB library was developed in a hobby project. There are known issues with it (e.g. no dynamic attachments, no error recovery, limited split support). For me it works well but that need not be the case with any device and in any situation when it goes out to the public.

The 10th Step
------------

In this step USB mouse support is added and demonstrated by a simple "painting program" in *sample/10-usbmouse/*. See the *README* file in this directory for details.

Please note that the class *CUSBStandardHub* must not be instanciated in *CKernel* anymore. This is done automatically from *CDWHCIDevice* now. Another change applies to the libraries used while linking. The new library *lib/input/libinput.a* must be used together with *lib/input/libusb.a*. In the samples this is already considered.

The options to be used for *cmdline.txt* are described in *doc/cmdline.txt*.

In Step 1-9 the following features were introduced:

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
* boot: Do *make* in this directory to get the Raspberry Pi firmware files required to boot.
* doc: Additional documentation files.

Classes
-------

The following C++ classes were added to the USB library or extended in the lib/usb/ subdirectory:

* CDWHCIRootPort: Supporting class for CDWHCIDevice, initializes the root port.
* CUSBDevice: Encapsulates a general USB device (detects the functions of this device).
* CUSBFunction: Encapsulates a function (represented by an interface descriptor) of an USB device.
* CUSBHIDDevice: General USB HID device (e.g. keyboard, mouse), boot protocol only
* CUSBMouseDevice: Driver for USB mice

In this step the *input library* is added which currently offers the following classes in the lib/input/ subdirectory:

* CKeyboardBehaviour: Generic keyboard function
* CKeyMap: Keyboard translation map (two selectable default maps at the moment)

The available Circle classes are listed in the file *doc/classes.txt*.
