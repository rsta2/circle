Circle
======

> This is Step 3 of Circle. To get access to Step 1-2 use the git tag "Step1" or "Step2".

> If you read this file in an editor you should switch line wrapping on.

Overview
--------

Circle is a C++ bare metal environment for the Raspberry Pi. Currently it should run on all existing models but I can only test it on model B and B+. Later these will be the only supported because of the required on-board USB hub.

Circle will be developed and released step by step. So it may be easier to understand it. At the beginning the functionality is very limited. The main goal is to setup the classes needed for the USB stack now.

The 3nd Step
------------

MMU and frame buffer are on by default now. First blink 5 times to show the image was loaded right. Write character set to screen. Write some logging messages to screen or UART. debug_hexdump() of the starting bytes of the ATAG structure at 0x100. Show usage of assert() and stack-trace.

You can create a file *cmdline.txt* like this on the SD(HC) card to change the frame buffer size:

`width=640 height=480`

In the same file you can control the logging feature by these options (append them to the same line):

`logdev=ttyS1 loglevel=4`

(write logging messages to UART now, default is to screen ("tty1"), the *loglevel* controls the amount of messages produced (0: only panic, 1: also errors, 2: also warnings, 3: also notices, 4: also debug output (default))

Circle has the following new features:

* Formatting strings
* Using devices
* Writing characters to screen
* Writing characters to UART
* Logging output to screen or UART
* Using assertions and debug hexdump

In Step 1-2 the following features were introduced:

* C++ build environment
* Simple delay functionality
* Get properties (model, memory size) from VideoCore
* new and delete
* Using GPIO pins
* Manipulating Act LED
* Set pixel on screen
* Use kernel options
* Switch on MMU

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
* doc: Additional documentation files.

Classes
-------

The following C++ classes were added to the Circle library in the lib/ subdirectory:

* CCharGenerator: Gives pixel information for console font
* CDevice: Base class for all devices
* CDeviceNameService: Devices can be registered by name and retrieved later by this name
* CLogger: Writing logging messages to a target device
* CScreenDevice: Writing characters to screen, some escape sequences (some are not yet implemented)
* CSerialDevice: Writing characters to UART
* CString: Simple string manipulation class, Format() method works like printf() (but has less formating options)

In Step 1-2 the following classes were introduced:

* CActLED: Switch the Act LED on and off, checks the Raspberry Pi model to use the right LED pin.
* CBcmMailBox: Simple GPU mailbox interface, currently used for the property interface.
* CBcmPropertyTags: Get several information from the GPU side or control something on this side.
* CGPIOPin: Encapsulates a GPIO pin, can be read, write or inverted. Simple initialization.
* CMemorySystem: Currently only initializes the heap to the right memory size. Will be extended.
* CTimer: Currently only two simple static delay functions which work without creating an instance.
* CBcmFrameBuffer: Frame buffer initialization, setting color palette for 8 bit depth.
* CKernelOptions: Providing kernel options from file cmdline.txt ("width=" and "height=").
* CMemorySystem: Enabling MMU if requested, switching page tables (not used here).
* CPageTable: Encapsulates a page table to be used by MMU.
* CScreenDevice: Manipulating the frame buffer contents (only setting pixels for now).
