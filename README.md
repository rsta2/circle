Circle
======

> This is Step 8 of Circle. To get access to Step 1-7 use the git tag "Step1" to "Step7".

> If you read this file in an editor you should switch line wrapping on.

Overview
--------

Circle is a C++ bare metal environment for the Raspberry Pi. The main library should be useable on all existing models (but tested on model B and B+ only) but the USB library is working only on model B and B+ for now.

There is a model-check in the USB library to prevent non-high-speed USB devices from overclocking because the USB physical layer is initialized to high-speed PHY. Furthermore there is an USB hub required to be connected to the root port to ensure device enumeration is working. This is always the case with model B and B+.

Please note that this fairly small USB library was developed in a hobby project. There are known issues with it (e.g. no dynamic attachments, no error recovery, limited split support). For me it works well but that need not be the case with any device and in any situation when it goes out to the public.

Circle is developed and released step by step. So it may be easier to understand. The main goal was to integrate USB support until now.

The 8th Step
------------

First blink 5 times to show the image was loaded right. After initializing the USB host controller the USB hub driver detects all attached USB devices and displays its identifiers (vendor, device and interface).

The new feature in this step is the support for USB keyboards. Just type on the keyboard and your input will be echoed to the screen.

In "cooked mode" (used by default) a keyboard translation map is used to translate the physical key code into the right ASCII code or in an escape sequence for special keys (see the file *doc/keyboard.txt*). There are only two of these language-dependend maps at the moment (UK English (currently untested) and German). Select the wanted map at the end of the file *include/circle/sysconfig.h* before building. The "Caps Lock" and "Num Lock" keys should work as expected but the driver is not able to switch the LEDs at the moment. If you press Ctrl-Alt-Del the system reboots.

In "raw mode" the physical key codes of the pressed keys are send to the key-pressed handler. To test this please change the #if in CKernel::Run() in the file *sample/kernel.cpp*. You can also use it to discover to physical key codes if you want to construct your own keymap (file *lib/usb/keymap_??.h*). Note that it depends on the keyboard how many keys it can detect simultaneous (maximum is 6).

The options to be used for *cmdline.txt* are described in *doc/cmdline.txt*.

Circle has the following new features:

* Detects low- and full-speed devices
* Driver for USB keyboards

In Step 1-7 the following features were introduced:

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

Building
--------

Building is normally done on PC Linux. You need a [toolchain](http://elinux.org/Rpi_Software#ARM) for the ARM1176JZF core. First edit the file *Rules.mk* and set the *PREFIX* of your toolchain commands. Alternatively you can create a *Config.mk* file (which is ignored by git) and set the *PREFIX* variable to the prefix of your compiler like this (don't forget the dash at the end):

`PREFIX = arm-none-eabi-`

Then go to the build root of Circle and do:

`./makeall clean`  
`./makeall`

The ready build *kernel.img* file should be in the sample/ directory.

You can also build Circle on the Raspberry Pi itself on Debian wheezy but you need some method to put the *kernel.img* file onto the SD(HC) card. With an external USB card reader on model B+ (4 USB ports) this should be no problem.

Building Circle from a non-Linux host is possible too. You need a cross compiler targetting (for example) *arm-none-eabi*. OSDev.org has an [excellent document on the subject](http://wiki.osdev.org/GCC_Cross-Compiler) that you can follow if you have no idea of what a cross compiler is, or how to make one.

Installation
------------

Copy the Raspberry Pi firmware (from boot/ directory, do *make* there to get them) files along with the kernel.img (from sample/ directory) to a SD(HC) card with FAT file system. Put the SD(HC) card into the Raspberry Pi.

Directories
-----------

* include: The common header files, most class headers are in the include/circle/ subdirectory.
* lib: The Circle class implementation and support files (USB library in the lib/usb/ subdirectory).
* sample: A sample application using Circle. The main function is implemented in the CKernel class.
* boot: Do *make* in this directory to get the Raspberry Pi firmware files required to boot.
* doc: Additional documentation files.

Classes
-------

The following C++ classes were added to the Circle USB library in the lib/usb/ subdirectory:

* CUSBKeyboardDevice: Driver for USB keyboards
* CKeyMap: Keyboard translation map (two selectable default maps at the moment)
* CDWHCIFrameSchedulerNonPeriodic: Schedules the transmission of non-interrupt split frames to non-high-speed devices
* CDWHCIFrameSchedulerPeriodic: Schedules the transmission of interrupt split frames to non-high-speed devices

In Step 1-4 the following classes were introduced for the Circle main library:

* CActLED: Switch the Act LED on and off, checks the Raspberry Pi model to use the right LED pin.
* CBcmMailBox: Simple GPU mailbox interface, currently used for the property interface.
* CBcmPropertyTags: Get several information from the GPU side or control something on this side.
* CGPIOPin: Encapsulates a GPIO pin, can be read, write or inverted. Simple initialization.
* CBcmFrameBuffer: Frame buffer initialization, setting color palette for 8 bit depth.
* CKernelOptions: Providing kernel options from file cmdline.txt (see *doc/cmdline.txt*).
* CMemorySystem: Enabling MMU if requested, switching page tables (not used here).
* CPageTable: Encapsulates a page table to be used by MMU.
* CCharGenerator: Gives pixel information for console font
* CDevice: Base class for all devices
* CDeviceNameService: Devices can be registered by name and retrieved later by this name
* CLogger: Writing logging messages to a target device
* CScreenDevice: Writing characters to screen, some escape sequences (some are not yet implemented)
* CSerialDevice: Writing characters to UART
* CString: Simple string manipulation class, Format() method works like printf() (but has less formating options)
* CExceptionHandler: Generates a stack-trace and a panic message if an abort exception occurs.
* CInterruptSystem: Connecting to interrupts, an interrupt handler will be called on interrupt.
* CTimer: Supports an uptime clock, kernel timers and a calibrated delay loop.

In Step 5-7 the following C++ classes were added to the Circle USB library in the lib/usb/ subdirectory:

* CDWHCIDevice: USB host controller interface (HCI) driver for Raspberry Pi.
* CUSBHostController: Base class of CDWHCIDevice, some basic functions for host controllers.
* CDWHCITransferStageData: Holds all the data needed for a transfer stage on one HCI channel.
* CDWHCIRegister: Supporting class for CDWHCIDevice, encapsulates a register of the HCI.
* CDWHCIFrameScheduler: Base class for a simple micro frame scheduler (not used so far but required to compile).
* CUSBDevice: Encapsulates a general USB device (basic device initialization, derived from CDevice).
* CUSBEndpoint: Encapsulates an endpoint of an USB device (supports control, bulk and interrupt EPs).
* CUSBRequest: A request to an USB device (URB).
* CUSBConfigurationParser: Parses and validates an USB configuration descriptor.
* CUSBStandardHub: USB hub driver for LAN9512/9514 hub (not tested with external hubs but may work)
* CUSBDeviceFactory: Creates the device objects of the different supported USB devices.
* CSMSC951xDevice: Driver for the on-board USB Ethernet device.
* CNetDevice: Base class of CSMSC951xDevice.
* CMACAddress: Encapsulates an Ethernet MAC address.
* CUSBBulkOnlyMassStorageDevice: Driver for USB mass storage devices (bulk only)
