Circle
======

Overview
--------

Circle is a C++ bare metal programming environment for the Raspberry Pi. It should be usable on all existing models (tested on model A+, B, B+, on Raspberry Pi 2, 3, 4, 400 and on Raspberry Pi Zero), except on the Raspberry Pi Pico, which is not supported. Circle provides several ready-tested [C++ classes](doc/classes.txt) and [add-on libraries](addon/README), which can be used to control different hardware features of the Raspberry Pi. Together with Circle there are delivered several [sample programs](sample/README), which demonstrate the use of its classes. Circle can be used to create 32-bit or 64-bit bare metal applications.

Circle includes bigger (optional) third-party C-libraries for specific purposes in addon/ now. This is the reason why GitHub rates the project as a C-language-project. The main Circle libraries are written in C++ using classes instead. That's why it is called a C++ programming environment.

Release 45.1
------------

This hotfix release fixes the HDMI sound driver (without VCHIQ), which did not work any more on the Raspberry Pi 4 with the recommended firmware. Furthermore is enables the relative path support in the FatFs library.

The 45th Step
-------------

This release comes with **support for USB audio streaming devices**, available **for Raspberry Pi 4, 400 and Compute Module 4** only. Supported should be devices, which are compliant with the "USB Device Class Definition for Audio Devices", Release 1.0 and 2.0. Only USB audio interfaces with 16-bit PCM audio and two channels (Stereo) are supported for output and input, and additionally with one channel (Mono) for input. There is no constant chunk size for USB sound devices and it is not configurable here. You should enable the system option `REALTIME` for applications, which use USB sound. Some devices also may require the option `usbpowerdelay=1000` in the file [cmdline.txt](doc/cmdline.txt) to enumerate successfully.

USB audio streaming devices often support multiple jacks for output and input and some method was required to select them. Furthermore these devices have Feature Units, which allow to set the volume for different audio channels or to mute the whole signal. Before there was no common API for such functions. This release adds the new feature of a **sound controller** for that purpose, which is provided by the class `CSoundController`. A pointer to the sound controller of an existing sound device (derived from the class `CSoundBaseDevice`) can be requested by calling `GetController()` on its device object. See the [Circle documentation](https://circle-rpi.readthedocs.io/en/latest/devices/audio-devices.html#sound-controller) for more information.

Please note that the sound controller is optional and currently only the following sound devices implement it: `CUSBSoundBaseDevice`, `CI2SSoundBaseDevice` (for PCM512x-based devices), `CVCHIQSoundBaseDevice`. Because implementations of sound controllers for new devices are expected in the future, which provide additional audio functions, the sound controller API may be extended or modified in coming releases.

The sound support has been moved from the base library to the new library *lib/sound/libsound.a* with the header files in *include/circle/sound/* (instead of *include/circle/*). If your application uses sound, you have to add the sound library to the `LIBS` variable in the *Makefile* and to update some `#include` statements for the sound classes.

The samples [29-miniorgan](sample/29-miniorgan/), [34-sounddevices](sample/34-sounddevices/) and [42-soundinput](sample/42-soundinput/) (former *42-i2sinput*) have been updated to use USB audio streaming devices. The samples 29 and 42 also demonstrate functions of the sound controller. The sound recorder in sample 42 generates compatible *.wav* files now. The default sample rate for these samples is 48000 Hz now, because it is supported by most USB sound cards. The new test [sound-controller](test/sound-controller/) may also be of interest for trying several sound features and the sound controller.

There is a new method `CDevice::UnregisterRemovedHandler()` for undoing the registration of **device remove handlers**. Calling `CDevice::RegisterRemovedHandler()` with a `nullptr` for this purpose does not work any more. There can be multiple device remove handlers for one device now.

Further improvements:

* The **LVGL submodule** has been updated to version 8.3.3.
* The **FatFs submodule** has been updated with two recent patches. Furthermore it supports the function `f_mkfs()` for USB mass-storage devices now. This requires the FatFs option `FF_USE_MKFS` to be enabled in [addon/fatfs/ffconf.h](addon/fatfs/ffconf.h).
* There is a new **driver for SSD1306-based displays** in [addon/display/](addon/display/).
* The new system option `USE_LOG_COLORS` can be defined to **use different ANSI colors** for different severities **in the system log**.

Bug fixes:

* Reading the USB HID report descriptor for `int3-0-0` devices did fail on some devices, when they were not configured yet. The USB HID support was not usable on these devices before.
* Some USB MIDI controllers use an USB interrupt endpoint for reporting MIDI events, instead of a bulk endpoint. These devices were not usable before.
* The serial bootloader "Flashy" did not work with the Bluetooth modules HC-05/-06.

This release has been built with a new recommended toolchain, which comes from a new webpage. See the link in the *Building* section below.

With this release a number of Circle applications **can be built using Clang/LLVM**. Please see [doc/clang-support.txt](doc/clang-support.txt) for details. This support is currently experimental.

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
|                       | Clang/LLVM support (experimental)                   |
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
|                       | SMI master (experimental)                           |
|                       | I2S sound output and input                          |
|                       | HDMI sound output (without VCHIQ)                   |
|                       | Hardware random number generator                    |
|                       | Watchdog device                                     |
|                       | Official Raspberry Pi touch screen                  |
|                       | VCHIQ interface and audio service drivers           |
|                       | BCM54213PE Gigabit Ethernet NIC of Raspberry Pi 4   |
|                       | Wireless LAN access                                 |
|                       |                                                     |
| USB                   | Host controller interface (HCI) drivers             |
|                       | Standard hub driver (USB 2.0 only)                  |
|                       | HID class device drivers (keyboard, mouse, gamepad) |
|                       | Driver for on-board Ethernet device (SMSC951x)      |
|                       | Driver for on-board Ethernet device (LAN7800)       |
|                       | Driver for USB mass storage devices (bulk only)     |
|                       | Driver for USB audio streaming devices (RPi 4 only) |
|                       | Drivers for different USB serial devices            |
|                       | Audio class MIDI input support                      |
|                       | Touchscreen driver (digitizer mode)                 |
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
|                       | LVGL (by LVGL Kft)                                  |
|                       | 2D graphics class in base library                   |
|                       |                                                     |
| Not supported         | Bluetooth                                           |
|                       | Camera                                              |
|                       | USB device (gadget) mode                            |

Building
--------

> For building 64-bit applications (AArch64) see the next section.

This describes building on PC Linux. See the file [doc/windows-build.txt](doc/windows-build.txt) for information about building on Windows. If building for the Raspberry Pi 1 you need a [toolchain](http://elinux.org/Rpi_Software#ARM) for the ARM1176JZF core (with EABI support). For Raspberry Pi 2/3/4 you need a toolchain with Cortex-A7/-A53/-A72 support. A toolchain, which works for all of these, can be downloaded [here](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads). Circle has been tested with the version *11.3.Rel1* (arm-gnu-toolchain-11.3.rel1-x86_64-arm-none-eabi.tar.xz) from this website.

First edit the file *Rules.mk* and set the Raspberry Pi version (*RASPPI*, 1, 2, 3 or 4) and the *PREFIX* of your toolchain commands. Alternatively you can create a *Config.mk* file (which is ignored by git) and set the Raspberry Pi version and the *PREFIX* variable to the prefix of your compiler like this (don't forget the dash at the end):

```
RASPPI = 1
PREFIX = arm-none-eabi-
```

The following table gives support for selecting the right *RASPPI* value:

| RASPPI | Target         | Models                   | Optimized for |
| ------ | -------------- | ------------------------ | ------------- |
|      1 | kernel.img     | A, B, A+, B+, Zero, (CM) | ARM1176JZF-S  |
|      2 | kernel7.img    | 2, 3, Zero 2, (CM3)      | Cortex-A7     |
|      3 | kernel8-32.img | 3, Zero 2, (CM3)         | Cortex-A53    |
|      4 | kernel7l.img   | 4B, 400, CM4             | Cortex-A72    |

For a binary distribution you should do one build with *RASPPI = 1*, one with *RASPPI = 2* and one build with *RASPPI = 4* and include the created files *kernel.img*, *kernel7.img* and *kernel7l.img*. Optionally you can do a build with *RASPPI = 3* and add the created file *kernel8-32.img* to provide an optimized version for the Raspberry Pi 3.

The configuration file *Config.mk* can be created using the `configure` tool too. Please enter `./configure -h` for help on using it!

> There are a number of configurable system options in the file [include/circle/sysconfig.h](include/circle/sysconfig.h). Please have a look into this file to learn, how you can configure Circle for your purposes. Some hardware configurations may require modifications to these options (e.g. using USB on the CM4).

Then go to the build root of Circle and do:

```
./makeall clean
./makeall
```

By default only the latest sample (with the highest number) is build. The ready build *kernel.img* file should be in its subdirectory of sample/. If you want to build another sample after `makeall` go to its subdirectory and do `make`.

You can also build Circle on the Raspberry Pi itself (set `PREFIX =` (empty)) on Raspbian but you need some method to put the *kernel.img* file onto the SD(HC) card. With an external USB card reader on model B+ or Raspberry Pi 2/3/4 model B (4 USB ports) this should be no problem.

AArch64
-------

Circle supports building 64-bit applications, which can be run on the Raspberry Pi 3 or 4. There are also Raspberry Pi 2 versions and the Raspberry Pi Zero 2, which are based on the BCM2837 SoC. These Raspberry Pi versions can be used too (with `RASPPI = 3`).

The recommended toolchain to build 64-bit applications with Circle can be downloaded [here](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads). Circle has been tested with the version *11.3.Rel1* (arm-gnu-toolchain-11.3.rel1-x86_64-aarch64-none-elf.tar.xz) from this website.

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
* test: Several test programs, which test different features of Circle.
* tools: Tools for building Circle and for using Circle more comfortable (e.g. a serial bootloader).

Classes
-------

The following C++ classes were moved in Circle:

Base library -> Sound library (new)

* CDMASoundBuffers: Concatenated DMA buffers to be used by sound device drivers
* CHDMISoundBaseDevice: Low level access to the HDMI sound device (without VCHIQ)
* CI2SSoundBaseDevice: Low level access to the I2S sound device
* CPWMSoundDevice: Using the PWM device to playback sound samples in different formats
* CPWMSoundBaseDevice: Low level access to the PWM device to generate sounds on the headphone jack
* CSoundBaseDevice: Base class of sound devices, converts several sound formats

The following C++ classes were added to Circle:

USB library

* CDWHCIFrameSchedulerIsochronous: Schedules the transmission of isochronous split-frames
* CUSBAudioControlDevice: Driver for USB audio control devices
* CUSBAudioFunctionTopology: Topology parser for USB audio class devices
* CUSBAudioStreamingDevice: Low-level driver for USB audio streaming devices

Sound library (new)

* CPCM512xSoundController: Sound controller for PCM512x
* CSoundController: Optional controller of a sound device
* CUSBSoundBaseDevice: High-level driver for USB audio streaming devices
* CUSBSoundController: Sound controller for USB sound devices
* CWM8960SoundController: Sound controller for WM8960

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
* [Clang support](doc/clang-support.txt)
* [Memory layout](doc/memorymap.txt)
* [Naming conventions](doc/naming-conventions.txt)
* [Known issues](doc/issues.txt)

Trademarks
----------

Raspberry Pi is a trademark of Raspberry Pi Ltd.

Linux is a trademark of Linus Torvalds.

PS3 and PS4 are registered trademarks of Sony Computer Entertainment Inc.

Windows, Xbox 360 and Xbox One are trademarks of the Microsoft group of companies.

Nintendo Switch is a trademark of Nintendo.

Khronos and OpenVG are trademarks of The Khronos Group Inc.

OpenGL ES is a trademark of Silicon Graphics Inc.

The micro:bit brand belongs to the Micro:bit Educational Foundation.

HDMI is a registered trademark of HDMI Licensing Administrator, Inc.
