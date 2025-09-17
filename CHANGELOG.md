Change Log
==========

This file contains the release notes (the major changes) since Circle Step30 for quick access. For earlier releases please checkout the respective git tag and look into README.md. More info is attached to the release tags (git cat-file tag StepNN) and is available in the git commit log.

Release 50.0.1
--------------

This is a hotfix release, which fixes a serious bug in the class `CmDNSDaemon`, which also affected the class `CmDNSPublisher`. If you are using one of these classes, you should upgrade.

The 50th Step
-------------

2025-08-15

This release comes with full (send and receive) network multicast support, including IGMPv2 support. A class `CmDNSDaemon` has been added, which determines and maintains our mDNS hostname on the local network. Name collisions with other hosts will be resolved by appending a numeric suffix to the hostname. The daemon is automatically started, when someone requests a pointer to it. This has been added to the `CmDNSPublisher` class, which works in conjunction with `CmDNSDaemon` now. The *test/mdns-publisher* uses the numeric suffix to generate an unique service name.

More news:

* The function of the `CTerminalDevice` sequences ESC "[0m" and ESC "[27m" have been modified. The first one resets the color settings too, while the second one resets the reversed mode only.
* The method `CLogger::Read()` returns 0 now, when the log buffer is empty. Before it returned -1.
* The DHCP process has been improved and is quicker now under some circumstances.
* A method `GetUptime(sec, usec)` has been added to the class `CTimer`.
* The FatFs library in *addon/fatfs/* has been updated to R0.16 with Unicode patch.
* A display font `Font12x22` has been added. The class `CCharGenerator` can handle fonts, with a width of up to 32 pixels.

Fixes:

* The HDMI sound support with the class `CHDMISoundBaseDevice` did not work since Step 49.
* The WLAN support in earlier versions was based on a port of wpa_supplicant v0.7.0, for which open security advisories exist. This has been fixed by porting wpa_supplicant v2.11 to Circle.
* The Handling of "Enumeration done" in the USB gadget driver had issues, that made booting from the USB mass-storage device (MSD) gadget impossible.

The recommended toolchain for building Circle is based on GCC 14.3.1 now (see the Building section).

The recommended firmware, downloadable in *boot/*, has been updated. For the Raspberry Pi 5 it is required to use the new DTB file(s), because otherwise some functions will not work with this Circle version.

The WLAN firmware, downloadable in *addon/wlan/firmware/*, has been updated. It is absolutely recommended to use the new firmware, because older versions may have security vulnerabilities.

Release 49.0.1
--------------

2025-06-18

This hotfix release solves the following issues:

* Download of firmware files in boot/ did not work any more.
* LVGL library in addon/lvgl/ did not build on Windows.
* "fgrep is obsolescent" warnings appeared on newer build host systems.

The 49th Step
-------------

2025-03-18

This release comes with an improved dot-matrix display management. All driver classes for dot-matrix displays should be derived from the class `CDisplay` now. The old character display support for ST7789- and SSD1306-based displays is still available, but will be deprecated in a future version. Instead there is the new class `CTerminalDevice`, which implements a scrolling character terminal display for any display driver, which is derived from `CDisplay`. This class is also used to implement the class `CScreenDevice` now for the known terminal display on a firmware-driven frame buffer device. The following displays are currently supported by `CDisplay`-derived driver classes:

* Firmware-driven frame buffer (`CBcmFrameBuffer`)
* ST7789 SPI display (`CST7789Display`)
* SSD1306 I2C display (`CSSD1306Display`)
* ILI9341 SPI display (`CILI9341Display`)

Beside the terminal support also the 2D graphics (`C2DGraphics`) and LVGL (`CLVGL`) support have been updated to work with all these displays. The 2D graphics support works with logical colors (`T2DColor`) now. There is a new class `C2DImage`, which manages the color conversion from logical to physical colors for 2D sprite images.

Classes, which support the displaying of text on dot-matrix displays, allow the selection of the used font now. The default system font can by defined with system option `DEFAULT_FONT`.

There is a new class `CWindowDisplay`, which allows to use multiple non-overlapping windows on a dot-matrix display. This is demonstrated in the multi-core program *sample/43-multiwindow*.

*sample/08-usbkeyboard*, *sample/41-screenanimations* and *addon/lvgl/sample* have been updated for the new display management and support SPI and I2C displays too.

There are a number of API breaking changes for the new display support, which are listed in [this article](https://github.com/rsta2/circle/discussions/380#discussioncomment-11417658).

More news:

* The external PCIe bus of the Raspberry Pi 5 can be accessed using the class `CBcmPCIeHostBridge` now. See the *test/pcie-external* for details. Interrupts from the external PCIe bus are available via the INTA pin at the IRQ number `ARM_IRQ_PCIE_EXT_HOST_INTA`.
* A driver for XPT2046-based touchscreens has been added. See *test/xpt2046-touchscreen* for details.
* The FatFs support has been updated to R0.15a + patch1.
* The LVGL support has been updated to v9.2.2.
* DMA channels are usable in different modes now. Before a DMA channel, which has been used for an asynchronous transfer, could not be used for synchronous transfers afterwards without re-initialization. The completion routine has to be set prior to each asynchronous transfer now.

Fixes:

* The detection of WM8960-based I2S codecs did not work, when the I2C address was explicitly specified.
* Commit ae00d9d8 in Step48 was leading to lost MIDI events with USB MIDI devices on the Raspberry Pi 1-3 and has been reverted. In the rare case that you are using an USB device, which has a MIDI interface and an other (e.g. HID) interface, and the device is directly connected to the root port without USB hub in-between (e.g. on Raspberry Pi Zero), you have to define the system option `USE_NAK_USB_FIX` now.
* The MQTT client might have crashed after receiving a disconnect from peer before.
* The check for the length, opcode and block number of incoming ACK packets in the TFTP daemon used an invalid logical operator. This could have caused receiving invalid files on read requests.
* The HideLink THEC64 USB keyboard did not work before.
* The initial LVGL mouse cursor was not centered on the Raspberry Pi 5.
* The `configure` script and cFlashy can be used on macOS now. cFlashy caused a build error before on macOS.

The recommended firmware version has been updated. The option `initial_turbo=0` has been added to the file *config.txt*, because newer firmware versions enable `initial_turbo=60` by default now, which can disturb the Circle device initialization.

The 48th Step
-------------

2024-11-08

This release comes with **USB mass-storage device gadget** support. *test/usb-msd-gadget* shows, how this can be used to provide direct access to the SD card via USB. Furthermore this version supports **HDMI sound for the Raspberry Pi 5**. The sound samples and tests have been updated to use this. The **Raspberry Pi 5 with BCM2712 stepping D0** is supported. The **LVGL support** has been updated to **version 9.2.0**.

The network library has been updated to support the receiving of ICMP packets in applications. This is used in *test/ping-client* to implement a **ping client**. Raspberry Pi models without an on-board Ethernet NIC can be used with **RTL8152 and RTL8153 USB Ethernet adapters** to access a network.

The platform **DMA** controller drivers support **cyclic write transfers** now. To implement this, it was necessary to extend the type `TDMACompletionRoutine` by a buffer index. If you directly use DMA transfers in your application, you have to update your DMA completion routine.

There is a complete rewrite of the Flashy **serial bootloader tool in C**, which is called cFlashy and which does not need a Node-JS environment any more. You can define `USEFLASHY=0` in the file *Config.mk* to enable it. See [doc/bootloader.txt](doc/bootloader.txt) for details.

More news:

* A class `CmDNSPublisher` was added, which can be used to publish services to mDNS (aka Bonjour) in a local network. See *test/mdns-publisher* for an example.
* There is a new option `backlight=` for *cmdline.txt* for setting the backlight level for the Official 7" Raspberry Pi touchscreen. For some versions of this touchscreen this option is required.
* The class `CSerialDevice` now supports modifying the parity setting (including a mark or space parity) and a per character receive for specific RS-485 applications.
* A block cursor can be enabled on the screen using `CScreenDevice::SetCursorBlock()`. See *test/screen-ansi-colors* for an example.
* The SD card driver implements the `GetSize()` method now.
* Only serious USB transaction errors are logged on Raspberry Pi 1-3 any more.
* The new doc file [doc/debug-swd.txt](doc/debug-swd.txt) explains SWD debugging on the Raspberry Pi 5 using the Raspberry Pi Debug Probe adapter.

Fixes:

* Only 4 GB RAM were usable, even on Raspberry Pi 5 with 8 GB RAM.
* The MAC address for WLAN access is read from the device tree on the Raspberry Pi 5 now, as it is required. Before always the same address was used.
* The Official 7" Raspberry Pi touchscreen did not work with circle-stdlib.
* The MQTT client delivered an invalid payload in `OnMessage()`, when there were multiple MQTT publish messages arriving in one network packet.
* The access to USB bulk endpoints on the Raspberry Pi 1-3 in no-hub configurations was occupying the whole USB bandwidth, which was not allowing to access other endpoints on the same USB device.
* The I2C master was not initialized on the Raspberry Pi 5 in *sample/34-sounddevices*.

The recommended firmware version has been updated and is required to use the Raspberry Pi 5 with BCM2712 stepping D0. Please note that the file *bcm2712d0.dtbo* has to be copied into the directory *overlays/* on the SD card.

The 47th Step
-------------

2024-07-09

This release provides a number of **new features for the Raspberry Pi 5**, which were already available for earlier models:

* SPI master support (polling and DMA driver)
* I2S sound (output or input, DMA or programmed I/O operation)
* PWM sound (requires external circuit on GPIO12/13 or GPIO18/19)
* PWM output (4 channels)
* GPIO clocks (GP0-2)
* `CGPIOPin::WriteAll()` and `CGPIOPin::ReadAll()`

The following **new hardware features of the Raspberry Pi 5** are supported now:

* Real-time clock (class `CFirmwareRTC` in [addon/rtc](addon/rtc))
* Power button (Function `is_power_button_pressed()`)
* Function `main()` can return `EXIT_POWER_OFF` to power-off the system
* 8-channels I2S sound output (via GPIO21/23/25/27, e.g. for HifiBerry DAC8x)

The **WM8960 I2S sound driver** has been revised and provides a better audio quality and the sound controller jack and control functions now. The new sound controller control `ControlALC` (Automatic Level Control) has been defined and implemented for the WM8960. ALC is disabled by default now. The WM8960 driver supports sample rates of 44100 and 48000.

More news:

* A new **IRQ-based driver for the I2C master** of Raspberry Pi 1-4 is available.
* A **character mode for ST7789-based dot-matrix displays** is available.
* The **timing of the GPIO pin driver** has been improved on the Raspberry Pi 5 by using the RIO module.
* There is a **new GPIO pin mode** `GPIOModeNone`, which disables the GPIO pin on the Raspberry Pi 5. On other models it has the same function as `GPIOModeInput`.
* The new static method `CGPIOPin::SetModeAll()` allows to **set the mode of the GPIO pins 0-31 at once** to input or output.
* **IP multi-cast support level 1** according to RFC 1112 is implemented (send only).
* The **LVGL support** has been updated to LVGL v8.3.11.

Fixes:

* The USB serial CDC gadget was not detected on Windows 10.
* The Raspberry Pi Debug Probe UART did not work with the USB serial CDC driver.
* The class `CPWMSoundDevice` did not apply the full chunk size.

The recommended firmware and toolchain versions have been updated.

The 46th Step
-------------

2024-02-28

With this release Circle initially **supports the Raspberry Pi 5**. There are many features, which are not available yet, but important features like USB and networking are supported. Please see the [Circle documentation](https://circle-rpi.readthedocs.io/en/46.0/appendices/raspberry-pi-5.html) for more information on Raspberry Pi 5 support!

Circle comes with an **USB serial CDC gadget** now, which allows to communicate with a Circle application from a host computer via a serial interface without an USB serial adapter. This can be tested with the [test/usb-serial-cdc-gadget](test/usb-serial-cdc-gadget/).

The **properties file library** in [addon/Properties](addon/Properties/) supports section headers now.

A possible race condition in `CTimer` has been fixed, which could only occur with the KY-040 rotary encoder module driver.

Release 45.3.1
--------------

2023-10-08

This is a hotfix release. It fixes the release of guard structures, which are used to protect static objects, which are defined inside of a function. This problem did occur only, when the system option `ARM_ALLOW_MULTI_CORE` was defined.

Release 45.3
------------

2023-10-06

This release comes with initial **USB gadget (aka device, peripheral) mode support**, which is used to implement an **USB MIDI (v1.0) gadget**. This allows to connect the Raspberry Pi models (3)A(+), Zero (2) (W) and 4B directly to a host computer (e.g. for running a sequencer program). Before the Raspberry Pi was always the USB host with Circle and required an additional USB MIDI serial adapter for that purpose.

The sample [29-miniorgan](sample/29-miniorgan/) is prepared to work as MIDI gadget. Please see the [README](sample/29-miniorgan/README) for information about the required configuration. Beside the define `USB_GADGET_MODE`, which enables the gadget mode in the sample, you have to define your own USB vendor ID as system option `USB_GADGET_VENDOR_ID` in *Config.mk* or *include/circle/sysconfig.h*. Please note that Circle does not support OTG protocols, so the USB controller always works in host or gadget mode and the connected peer must work in the opposite mode.

Adapting your own application to be used as an USB MIDI gadget should not be difficult. You have to create an object of the class `CUSBMIDIGadget` (see *include/circle/usb/gadget/usbmidigadget.h*) instead of `CUSBHCIDevice` and call `Initialize()` and `UpdatePlugAndPlay()` on it as before in host mode. You have to add the library *lib/usb/gadget/libusbgadget.a* to your `LIBS` variable. The USB MIDI API device `umidi1` has the same interface as in host mode. There is a shared base class `CUSBController` for `CUSBHCIDevice` and `CUSBMIDIGadget`, so it is easy to implement host and gadget mode in one application and to select it on user configuration.

Further improvements:

* The **LVGL submodule** has been updated to version 8.3.10.
* **Application-defined kernel options** can be used now in the file *cmdline.txt*. The methods `GetAppOptionString()` and `GetAppOptionDecimal()` have been added to the class `CKernelOptions` for this purpose.
* **Resizing the screen** is supported in the classes `CScreenDevice`, `C2DGraphics` and `CMouseDevice`.
* **TV service support** has been added to [addon/vc4/interface](addon/vc4/interface/). It works in 32-bit mode only.
* The class `CI2CMaster` supports **I2C operations with repeated start** now.

Release 45.2
------------

2023-05-22

This release provides many enhancements and fixes for the **USB audio streaming support** for Raspberry Pi 4, 400 and Compute Module 4. USB audio interfaces with 16-bit or 24-bit wide samples with up to 32 channels are supported now. You have to specify the option `soundopt=24` in the file *cmdline.txt* to select 24-bit wide samples and the option `usbsoundchannels=TX,RX` to select a specific number of output (TX) and input (RX) channels. See the file [cmdline.txt](doc/cmdline.txt) for more info on this option. You can call the methods `GetHWTXChannels()` and `GetHWRXChannels()` of a sound device driver class to request the number of available hardware channels in your application now.

Please note that an application, which uses the alternate sound interface and wants to use USB audio streaming support with 24-bit wide samples, has to implement the method `unsigned GetChunk(u32 *pBuffer, unsigned nChunkSize)`, were each sample occupies 3 bytes. The class `CUSBSoundBaseDevice` must be instantiated, when the USB host controller driver has been initialized already. Therefore you cannot do this in the constructor of `CKernel` and must create the driver object later using the `new` operator.

There is support for the Raspberry Pi **Camera Modules 1 and 2** now in the external project [libcamera](https://github.com/rsta2/libcamera).

Further improvements:

* The **LVGL submodule** has been updated to version 8.3.7. The `CLVGL` wrapper class has been updated for better performance and compatibility with LVGL.
* The **FatFs submodule** has been updated to release R0.15 with patch 1 and 2.
* The support for USB serial interfaces has been extended for **more CP210x family devices**.
* Support for **Ctrl+navigation key combinations for USB keyboard** devices in cooked mode has been added.
* Support for **rotated and/or mirrored display for SSD1306-based dot-matrix displays** has been added.
* A `C2DGraphics::DrawText()` method for **displaying text with 2D graphics** has been added.
* **gzip-compressed kernel images** (supported for 64-bit kernels only) can be generated by defining `GZIP_KERNEL = 1` in *Config.mk* now.

Bug fixes:

* DMA4 (`DMA_CHANNEL_EXTENDED`) memory-to-memory transfers sometimes failed.
* `C2DGraphics::DrawImageRect()` used display stride instead of image stride.

The **recommended firmware** has been updated and can be downloaded in [boot/](boot/). It is now always recommended to copy the file *config32.txt* (AArch32) or *config64.txt* (AArch64) from the *boot/* directory to the SD card and to rename it to *config.txt* there.

The **recommended toolchain** is based on GCC 12.2.1 now. You can download it using the link in the *Building* section below. With this toolchain the system options `SAVE_VFP_REGS_ON_IRQ` and `SAVE_VFP_REGS_ON_FIQ` are enabled by default in any case now.

Please note that it is checked on start-up now, if the system option `KERNEL_MAX_SIZE` is properly set. If the given value (default 2 MByte) is too small, the system does not boot.

Release 45.1
------------

2023-02-01

This hotfix release fixes the HDMI sound driver (without VCHIQ), which did not work any more on the Raspberry Pi 4 with the recommended firmware. Furthermore is enables the relative path support in the FatFs library.

The 45th Step
-------------

2022-12-01

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

Release 44.5
------------

2022-06-08

This intermediate release offers a **revised DWHCI USB low-level driver for the Raspberry Pi 1-3 and Zero**. With the system option `USE_USB_FIQ` one can use the FIQ (Fast Interrupt Request) for this driver, which results in a more accurate timing on the USB. This may improve the compatibility with some USB devices and may help to prevent data loss, especially when receiving MIDI data from some USB MIDI controllers, which do have only small data buffers. Because there is only one FIQ source supported in the system, the FIQ cannot be used for other purpose than the USB with this system option. The xHCI USB driver for the Raspberry Pi 4 does not support this system option and remains unchanged.

To **prevent data loss from USB MIDI controllers on the Raspberry Pi 1-3 and Zero**, there is also the new option `usbboost=true` for the file *cmdline.txt* now. It speeds up the USB MIDI handling, but may generate more system load on the other hand.

The system option `USE_EMBEDDED_MMC_CM4` has been renamed to `USE_EMBEDDED_MMC_CM` and is tested to support **embedded MMC memory** on the Compute Module 3+ and 4.

The `CI2SSoundBaseDevice` class driver for I2S sound devices **supports the WM8960 DAC**.

There is **I2C support in the HD44780 LCD display driver** now.

Bug fixes:

* The Stereo channels were swapped in the `CHDMISoundBaseDevice` class before.
* There seem to be USB devices, which send more data than it is expected. This fix should prevent a system crash by faking a frame overrun error, which should be handled by the upper layers.
* Building the WLAN support with the `NDEBUG` option was not possible.

Don't forget to update the used firmware to the one downloadable in [boot/](boot/)!

Release 44.4.1
--------------

2022-03-12

This hotfix release fixes an issue in the initialization for 32-bit multi-core support. The start of the secondary CPU cores may have failed, if the data cache has not been flushed already, when `CMultiCoreSupport::Initialize()` was called. This especially happened, if the constructor of the class `CMultiCoreSupport` was executed immediately before `CMultiCoreSupport::Initialize()`.

Release 44.4
------------

2022-03-11

This intermediate release updates the FatFs and LVGL support to the most recent versions. While the **update to FatFs R0.14b** is compatible with earlier releases, the **update to LVGL v8.2.0** requires modifications in existing applications, which use this graphics library. Also there isn't a separate submodule *lv_examples* for LVGL examples any more, the demo program is included in the main submodule.

New features are:

* A **multi-channel driver and sample for WS2812 LED stripes**, which is based on a new **driver for the Secondary Memory Interface (SMI)**. See [addon/WS28XX/sample/multichan/](addon/WS28XX/sample/multichan/) for details!
* A **driver and sample for the KY-040 rotary encoder** in [addon/sensor/sample/ky040/](addon/sensor/sample/ky040/).
* The [sample/42-i2sinput](sample/42-i2sinput) has been extended with a **sound recorder mode**. You can record digital I2S sound from other devices to the SD card using this sample.
* **Support for the Raspberry Pi 4 Case Fan** has been added to the class `CCPUThrottle`. There is a new option `gpiofanpin=` for [cmdline.txt](doc/cmdline.txt), which enables the case fan support. The CPU speed is not throttled any more, when this option is used.

Don't forget to update the used firmware to the one downloadable in [boot/](boot/)!

Have a look at the new [Circle documentation](https://circle-rpi.readthedocs.io/)! Feel free to use the the new [Discussions forum](https://github.com/rsta2/circle/discussions) for topics, which are not Issues.

Release 44.3
------------

2021-12-02

This intermediate release supports the new **Raspberry Pi Zero 2 W**. Please download the updated recommended firmware in [boot/](boot/) to be able to use it!

**Tasks have a name** now, which can be assigned using `CTask::SetName()` and is shown in the task list, which can be generated with `CScheduler::ListTasks()`. A task can be found with this name using `CScheduler::GetTask()` and can be suspended from running with `CTask::Suspend()` and reactivated later with `CTask::Resume()`.

The I2S driver can be used via the **P5 extension header on early Raspberry Pi models**.

There are some new features in [addon/](addon/):

* Driver for **ST7789-based dot-matrix displays** with SPI interface in [addon/display/](addon/display/)
* Experimental **WLAN AP mode support** for open networks with static IP addresses in [addon/wlan/](addon/wlan/)
* **RAM loader for the Raspberry Pi Pico**, which uses the SWD debug interface in [addon/pico/](addon/pico/)
* **MCP3004/3008 driver** can return raw values in [addon/sensor/](addon/sensor/)

These fixes have been applied:

* The TCP sender did not set the PUSH flag under some circumstances, where it was necessary.
* When an synchronous xHCI USB transfer timed out, this might have lead to a failed assertion, when the next transfer starts.
* The DWHCI USB driver might have asserted, when a control message transfer failed before.

There is the new option `usbignore=` for the file *cmdline.txt*, which allows to explicitly ignore an USB device or interface in the USB device enumeration process, which is otherwise supported.

Release 44.2
------------

2021-10-28

This intermediate release supports **USB HID-class touchscreens** in digitizer mode now, which has been tested with the Waveshare 5 inch LCD B and 7 inch LCD C displays. Such USB touchscreens will be detected automatically, when the USB support is included in an application, and an instance of the class `CTouchScreenDevice` will be created, which provides the generic API for touchscreens. Please note that the class `CTouchScreenDevice` was the driver for the official 7 inch Raspberry Pi touchscreen before, which has been renamed to `CRPiTouchScreen` now. Existing applications, which support the official 7 inch Raspberry Pi touchscreen, have to be modified this way. [sample/28-touchscreen](sample/28-touchscreen) and the LVGL and uGUI wrappers have been updated to support both the official 7 inch Raspberry Pi touchscreen and USB HID-class touchscreens in digitizer mode. There is a new calibration tool in *tools/touchscreen-calibrator*, which allows to gather the calibration info for USB touchscreens, which require calibration.

The I2S sound device driver supports **I2S sound input** now. This is demonstrated in [sample/42-i2sinput](sample/42-i2sinput). See the file *README* in this directory for more info on using this sample program. The recognized I2S format is the same as the format, which is generated by the driver for output (2 channels with 32-bit width and 24-bit signed data).

There has been an inconsistency with `SoundFormatSigned24` in the sound drivers, because there are two different variants of it: one 24-bit format, which occupies 3 bytes per sample and one, which occupies 4 bytes per sample (value of one byte ignored). Previously the 4-bytes-format was used for `CI2SSoundBaseDevice::Write()`, if `SoundFormatSigned24` was selected as write format with 2 channels. For all other sound devices the 3-bytes-format was used. To fix this, a new `SoundFormatSigned24_32` is introduced, which is the 4-bytes-format. `SoundFormatSigned24` is now the 3-byte format in any case. In existing applications, which use `CI2SSoundBaseDevice::Write()`, it may be necessary to change the parameter of `SetWriteFormat()` to `SoundFormatSigned24_32`.

Further improvements:

* Class `CScreenDevice` supports ANSI colors (see [doc/screen.txt](doc/screen.txt)).
* Class `CSerialDevice` supports setting specific line parameters now and registers different device names, when used with `nDevice > 0` on the Raspberry Pi 4.
* Add class `CBcmWatchdog`, which controls the watchdog device of the Raspberry Pi.
* Add MCP3004/3008 DAC driver and sample to [addon/sensor](addon/sensor).
* Update LVGL graphics support to v7.11.0. The screen resolution is variable now up to 1920x1080.
* The USB CDC-class serial driver (class `CUSBSerialCDCDevice`) should be compatible with Arduino devices now.

Don't forget to update the used firmware to the one downloadable in [boot/](boot/)! The recommended toolchain is GCC 10.3.1 based now.

Release 44.1
------------

2021-06-14

This hotfix release solves the following issues:

* Do not use platform DMA12 controller, which has no dedicated IRQ line (Raspberry Pi 1-3 and Zero).
* M/S mode in class CPWMOutput did not work on channel 2.
* memmove() did not work together with circle-stdlib project. Now always implemented in Circle itself.
* doc/qemu.txt updated to refer to patched QEMU v6.0.0 to be used with Circle.
* doc/stdlib-support.txt referred to removed build.bash script.

The 44th Step
-------------

2021-05-14

This release comes with new features, improvements and bug fixes. There is a new HDMI sound driver class `CHDMISoundBaseDevice`, which allows to generate **HDMI sound without VCHIQ** driver, which can be easier to integrate in an application. This is shown by the [sample/29-miniorgan](sample/29-miniorgan) and [sample/34-sounddevices](sample/34-sounddevices). On the Raspberry Pi 4 only the connector HDMI0 is supported. The class `CI2SSoundBaseDevice` now supports the **PCM5122 DAC**.

A new class ``C2DGraphics`` has been added to the base library, which provides **2D drawing routines**, which work without flickering or screen tearing. This is demonstrated in the [sample/41-screenanimations](sample/41-screenanimations).

The **scheduler library** has been improved and provides the new classes `CMutex` and `CSemaphore`. Multiple tasks can wait for a `CSynchronzationEvent` to be set now.

There is a **new serial bootloader and flash tool** (Flashy), which improves the download speed and reliability. Please see the second part of the file [doc/bootloader.txt](doc/bootloader.txt) for more information! You can interrupt the download process with Ctrl-C now and start again, without resetting your Raspberry Pi. You should update your bootloader kernel image(s) on the SD card in any case. The old flash tool is still available.

Circle comes with a **configure script** now, which can be used to create the configuration file `Config.mk` easier. Please enter `configure -h` for a description of its options.

The C++ support has been improved. Now **placement new operators** and **static objects inside of a function** can be used. Furthermore the **C++17 standard** is optionally supported and can be enabled with the option `--c++17` of `configure`, if you have a toolchain version, which supports it.

Further improvements:

* There is a new system option `NO_BUSY_WAIT`. With this option enabled, the EMMC, SDHOST and USB drivers will **not busy wait for the completion of synchronous transfers** any more. This should improve system throughput and network latency, but requires the scheduler in the system.
* The **embedded MMC memory of the Compute Module 4** can be accessed, when the system option `USE_EMBEDDED_MMC_CM4` has been defined.
* The class `CTFTPFatFsFileServer` was added to [addon/tftpfileserver](addon/tftpfileserver) to support **TFTP access with the FatFs filesystem module**.
* The class `CDS18x20` in [addon/OneWire](addon/OneWire) has been improved and is now part of the library, not of the sample as before. It determines the used power mode of the sensor automatically.
* Functions for **atomic memory access** have been added to `<circle/atomic.h>`.

Bug fixes:

* System timer IRQ handling may have stopped working after a while on the Raspberry Pi 1 and Zero before.
* xHCI USB controller did not work on some Raspberry Pi 4 models.
* Starting secondary cores 1-3 was not reliable.
* Access to USB mass-storage devices was not reliable on Raspberry Pi Model A+, 3A+ and Zero before.
* Add workaround for non-compliant low-speed USB devices with bulk endpoints.
* Suppress concurrent split IN/OUT requests on Raspberry Pi 1-3 and Zero in USB serial drivers.
* Enable serial FIFO in polling mode too.
* The screen size select-able in *cmdline.txt* was limited to 1920x1080 before.
* Semaphore implementation in *addon/linux* was not IRQ safe, but used from IRQ handler in VCHIQ driver.
* Allow received text segment in TCP state SYN-RECEIVED.

Don't forget to update the used firmware to the one downloadable in [boot/](boot/)!

Release 43.3
------------

2021-01-28

This intermediate release adds support for the **Raspberry Pi 400** and partial support for the **Compute Module 4**. The eMMC memory of the Compute Module 4 cannot be accessed at the moment.

On the Compute Module 4 the **internal xHCI controller** can be used to access the USB. You have to enable the system option `USE_XHCI_INTERNAL` in *include/circle/sysconfig.h* and to add the option `otg_mode=1` to *config.txt* for this purpose. The DWC OTG USB controller is not supported on the CM4.

The **performance of SD card access** has been improved by enabling the High Speed mode of the SD card, if supported. With a suitable uSD XC card, the read performance increased to up to 180%.

**Bug fixes** concerning USB plug-and-play, the deferred start of the network subsystem and some HID class standard gamepads have been applied.

Circle is tested using a GCC 10.2.1 based toolchain now. Be sure to update the used firmware to the files downloadable in the *boot/* subdirectory, when using this Circle release!

Release 43.2
------------

2020-11-24

This intermediate release comes with new drivers and improvements in detail. First a number of **USB serial adapters** with different chips (CH341, CP2102, PL2303, FTDI) is supported now. These drivers are primarily intended to be used with serial client-server protocols, because a handshake (flow control) is not implemented so far. Only the tested combinations of USB vendor/device ID have been added to the drivers. If you discover, that your adapter with different IDs is working too, please let us know! The USB vendor/device ID combination has to be added in `GetDeviceIDTable()` at the bottom of the respective file *lib/usb/usbserial\*.cpp*.

These new USB serial drivers allow to provide **serial client libraries** and sample programs, which work with the [**RTK.GPIO**](addon/gpio) board and the [**micro:bit**](addon/microbit) computer. Please follow the given links for details.

There is an important change because of the **updated firmware version**, which is recommended for Circle. For **FIQ support on the Raspberry Pi 4 in AArch32 mode** a specific ARM stub file *armstub7-rpi4.bin* is required now. You will need a *config.txt* file for 32-bit operation now too for this configuration, as it already has been the case for 64-bit operation. Please read the file [boot/README](boot/README) for further information.

The USB mouse driver supports a **mouse wheel** and up to five buttons now. [sample/10-usbmouse](sample/10-usbmouse) has been updated to demonstrate this. This required a small modification to the mouse API. If your application uses a mouse, you probably have to add the `nWheelMove` (int) parameter to your mouse handler's prototype.

The Raspberry Pi 4 supports **multiple displays**. This can be used in Circle to provide more than one `CBcmFrameBuffer` and `CScreenDevice` instances now. If you want to use this feature, you have to add the setting `max_framebuffers=2` to your *config.txt* file on the SD card.

The LittlevGL project has been renamed by its authors to **LVGL**. Because the support for this project in Circle has been updated to the **new version 7.6.1**, the respective directory [addon/lvgl](addon/lvgl) and the class `CLGVL` now follows this convention. This support enables USB plug-and-play now.

A **number of fixes** has been applied for USB plug-and-play support on the Raspberry Pi 1-3.

Some time has been spent to provide **build support on Windows**. As described in [doc/windows-build.txt](doc/windows-build.txt), it is possible to build all Circle libraries and samples without modification now on this platform.

Finally some information about **JTAG debugging** in AArch64 mode on the Raspberry 3 and 4 have been added. Please see [doc/debug-jtag.txt](doc/debug-jtag.txt).

Release 43.1
------------

2020-10-10

This intermediate release adds **USB plug-and-play** support to the classes `CConsole` and `CUGUI` and related samples (*sample/32-i2cshell* and *addon/ugui/sample*).

Furthermore two **important bug fixes** have been applied. The first one affects the handling of interrupts in the xHCI USB driver for the Raspberry Pi 4, where the interrupts and thus all data transfers might have been stalled after a random amount of time. The second one prevents the access to deleted USB device object, when an USB device is surprise-removed from the Raspberry Pi 1-3 or Zero.

Some effort have been spent to allow **reducing the boot time**, when using the USB driver and the network subsystem. This is shown in [sample/18-ntptime](sample/18-ntptime). Have a look at the [README file](sample/18-ntptime/README) for details.

The **make target "tftpboot"** has been added to *Rules.mk*. If you have installed the *sample/38-bootloader* on your Raspberry Pi with Ethernet interface and have configured the host name (e.g. "raspberrypi") or IP address of it as `TFTPHOST=` in the file *Config.mk*, you can build and start a test program in a row using `make tftpboot`.

The 43rd Step
-------------

2020-10-02

This release adds **USB plug-and-play** (USB PnP) support to Circle. It has been implemented for all USB device drivers, which can be subject of dynamic device attachments or removes, and for a number of sample programs. Existing applications have to be modified to support USB PnP, but this is not mandatory. An existing application can continue to work without USB PnP unmodified. Please see the file [doc/usb-plug-and-play.txt](doc/usb-plug-and-play.txt) for details on USB PnP support and [sample/README](sample/README) for information about which samples are USB PnP aware!

USB PnP requires the **system option USE_USB_SOF_INTR** to be enabled in [include/circle/sysconfig.h](include/circle/sysconfig.h) on the Raspberry Pi 1-3 and Zero. Because it has proved to be beneficial for most other applications too, it is enabled by default now. Rarely it may be possible, that your application has disadvantages from it. In this case you should disable this option and go back to the previous setting (e.g. if you need the maximum network performance).

An important issue has been fixed throughout Circle, which affected the **alignment of buffers used for DMA operations**. These buffers must be aligned to the size of a data-cache line (32 bytes on Raspberry 1 and Zero, 64 bytes otherwise) in base address and size. In some cases your application may need to be updated to meet this requirement. For example this applies to the samples *05-usbsimple*, *06-ethernet* and *25-spidma*. Please see the file [doc/dma-buffer-requirements.txt](doc/dma-buffer-requirements.txt) for details!

Another problem in the past was, that the output to screen or serial device affected the **IRQ timing of applications**. There is the system option `REALTIME`, which already improved this timing. Unfortunately it was not possible to use low- or full-speed USB devices (e.g. USB keyboard) on the Raspberry Pi 1-3 and Zero, when this option was enabled. Now this is supported, when the system option `USE_USB_SOF_INTR` is enabled together with `REALTIME`.

The new **class CWriteBufferDevice** can be used to buffer the screen or serial output in a way, that writing to these devices is still possible at *IRQ_LEVEL*, even when the option `REALTIME` is defined. Using the new **class CLatencyTester** it is demonstrated in the new [sample/40-irqlatency](sample/40-irqlatency), how this affects the IRQ latency of the system. Please read the [README file](sample/40-irqlatency/README) of this sample for details!

Release 42.1
------------

2020-08-22

This intermediate release adds support for the **DMA4 "large address" channels** of the Raspberry Pi 4, which is available using the already known `CDMAChannel` class with the channel ID `DMA_CHANNEL_EXTENDED`. Despite the possibility to access more address bits, which is currently not used in Circle, the DMA4 channels provide a higher performance (compared to the legacy DMA channels). All (normally three available) DMA4 channels are free for application usage.

Newer **Raspberry Pi 4 models (with 8 GB RAM)** do not have a dedicated EEPROM for the firmware of the xHCI USB controller. They need an additional property mailbox call for loading the xHCI firmware after PCIe reset. This call has been added.

There has been no possibility for **application TCP flow control** before. An application sending much TCP data very fast was able to overrun the (low) heap, which caused a system halt. Now if `CSocket::Send()` is called with the flags parameter set to 0, the calling task will block until the TX queue is empty. This prevents the heap from overrun, but may slow down the transfer to some degree. If maximum transfer speed is wanted, the flags parameter can be set to `MSG_DONTWAIT`, but heap overrun must be prevented differently then (e.g. by sending less data).

Another fix has been applied to **network name resolution** in the class `CDNSClient`, which might have failed before, if the DNS server was sending uncompressed answer records.

The 42nd Step
-------------

2020-05-09

This release adds **Wireless LAN access** support in [addon/wlan](addon/wlan) to Circle. Please read the [README file](addon/wlan/sample/README) of the sample program for details! The WLAN support in Circle is still experimental.

To allow parallel access to WLAN and SD card, a new **SDHOST driver** for SD card access on Raspberry Pi 1-3 and Zero has been added. You can return to the previous EMMC interface in case of problems (e.g. if using QEMU) or for real-time applications by adding `DEFINE += -DNO_SDHOST` to *Config.mk*. WLAN access is not possible then. On Raspberry Pi 4 the **EMMC2 interface** is used for SD card access now.

Release 41.2
------------

2020-03-08

This intermediate release comes with some new features, improvements in detail and fixes. For the **Raspberry Pi 4** now the four **additional peripheral devices** each are supported for I2C master, SPI master and UART. The **SPI slave** is supported too on the latest Raspberry Pi model now.

Time has been spent to **improve the network library**. The **ARP** problem, which caused a delayed start of network transmissions, has been solved. ARP requests are retried now if necessary and the first packet sent in a new connection after reboot is not discarded any more. **ICMP** handling has been improved too. The **DHCP client** sends an configurable host name (default "raspberrypi") now in requests, so that the DHCP server can add a DNS entry for your Raspberry Pi, if it this enabled in the server.

There is a new **software profiling support** library in [addon/profile](addon/profile) for single core applications. The file *gmon.out*, which is generated by the software profiling library, is compatible with the *gprof* tool.

Previously a **CMemorySystem** object had to be instanced as first member of CKernel with `STDLIB_SUPPORT < 2` to be able to configure MMU usage by the application. This caused several problems. Newer compilers may generate code in the early stages, which does not work without MMU. Also constructors of static objects were called with MMU off before. Now that the CMemorySystem object is constructed early, these problems will be solved and all applications run with MMU on. For compatibility it is not a problem, if the application constructs a second instance of CMemorySystem in CKernel. It is used as an alias for the first instance.

The documentation generated by **Doxygen** with `./makedoc` has been improved by integrating and linking available documentation files. Important class definitions from the *addon/* subdirectory are included in the generated documentation now.

The following **issues** have been **fixed**:

* The BCM54213PE Gigabit Ethernet driver for the Raspberry Pi 4 did not reset the device properly. This caused some heavy re-transmissions of frames after chain boot (e.g. with sample/38-bootloader).

* AArch64 FIQ support did not initialize or close properly on Raspberry Pi 4 in some cases.

* It turned out, that the PWM audio channel outputs are swapped on some Raspberry Pi models. This will be considered automatically now. If you have implemented your own PWM audio driver, you can request from the CMachineInfo class, if the channels are swapped.

* Multi-core applications might have freezed when calling CSocket::Connect().

* CNTPDaemon did only save a pointer to the host name of the NTP server before. This might have caused problems, if the string buffer disappeared after returning from the constructor.

Release 41.1
------------

2020-02-14

This is a hotfix release, which fixes the bootloader. It did not work with the recommended firmware on Raspberry Pi 1 and Zero any more.

The 41st Step
-------------

2020-02-04

With this release Circle supports nearly all features on the Raspberry Pi 4, which are known from earlier models. Only OpenGL ES / OpenVG / EGL and the I2C slave support are not available.

On Raspberry Pi 4 models with over 1 GB SDRAM Circle provides a separate *HEAP_HIGH* memory region now. You can use it to dynamically allocate memory blocks with `new` and `malloc()` as known from the low heap. Both heaps can be configured to form a larger unified heap using a system option. Please read the file *doc/new-operator.txt* for details about using and configuring the heaps.

The new *sample/39-umsdplugging* demonstrates how to use `CUSBHCIDevice::RescanDevices()` and `CDevice::RemoveDevice()` to be able to attach USB mass-storage devices (e.g. USB flash drives) and to remove them again on application request without rebooting the system.

Some support for the QEMU semihosting interface has been added, like the possibility to exit QEMU on `halt()`, optionally with a specific exit status (`set_qemu_exit_status()`). This may be used to automate tests. There is a new class `CQEMUHostFile`, which allows reading and writing files (including stdin / stdout) on the host system, where QEMU is running on. See the directory *addon/qemu/*, the new sample in *addon/qemu/hostlogdemo/* and the file file *doc/qemu.txt* for info on using QEMU with Circle with the semihosting API.

The Circle build system checks dependencies of source files with header files now and automatically rebuilds the required object files. You will not need to clean the whole project after editing a header file any more. You have to append the (last) line `-include $(DEPS)` to an existing Makefile to enable this feature in your project. The dependencies check may be globally disabled, by defining `CHECK_DEPS = 0` in Config.mk.

If you want to modify a system option in *include/circle/sysconfig.h*, explicitly changing this file is not required any more, which makes it easier to include Circle as a git submodule. All system options can be defined in *Config.mk* this way:


```
DEFINE += -DARM_ALLOW_MULTI_CORE
DEFINE += -DNO_CALIBRATE_DELAY
```

Finally a project file for the Geany IDE is provided in *tools/template/*. The recommended toolchain is based on GNU C 9.2.1 now. As announced the Bluetooth support has been removed for legal reasons.

Release 40.1
------------

2019-12-15

This intermediate release mostly adds FIQ support for AArch64 on the Raspberry Pi 4. This has been implemented using an additional ARM stub file *armstub8-rpi4.bin*, which is loaded by the firmware. The configuration file *config.txt*, provided in *boot/*, is required in any case for AArch64 operation now. Please read the file *boot/README* for detailed information on preparing a SD card with the firmware for using it with Circle applications.

*boot/Makefile* downloads a specific firmware revision now. With continuous Raspberry Pi firmware development there may occur compatibility problems between a Circle release and a new firmware revision, which may lead to confusion, if they are not detected and fixed immediately. To overcome this situation a specific tested firmware revision is downloaded by default now. This firmware revision reference will be updated with each new Circle release.

Further news in this release are:

* User timer (class CUserTimer) supported on Raspberry Pi 4
* LittlevGL support in *addon/littlevgl/* updated to v6.1.1
* FatFs support in *addon/fatfs/* updated to R0.14

Finally the system option *SCREEN_HEADLESS* has been added to *include/circle/sysconfig.h*. It can be defined to allow headless operation of sample programs especially on the Raspberry Pi 4, which would otherwise fail to start without notice, if there is not a display attached (see the end of next section for a description of the problem).

The 40th Step
-------------

2019-09-02

This Circle release adds support for the Raspberry Pi 4 Model B. All features supported by previous releases on Raspberry Pi 1-3 should work on Raspberry Pi 4 except:

* Accelerated graphics support
* FIQ on AArch64
* User timer (class CUserTimer)
* I2C slave (class CI2CSlave)
* USB RescanDevices() and RemoveDevice()

The Raspberry Pi 4 provides a number of new features. Not all of these are supported yet. Support for the following features is planned to be added later:

* USB 3.0 hubs
* High memory (over 1 GByte)
* additional peripherals (SPI, I2C, UART)

The Raspberry Pi 4 has a new xHCI USB host controller and a non-USB Gigabit Ethernet controller. This requires slight modifications to existing applications. The generic USB host controller class is named `CUSBHCIDevice` and has to be included from `<circle/usb/usbhcidevice.h>` now. The Ethernet controller driver is automatically loaded, if the TCP/IP network library is used, but has to be loaded manually otherwise (see *sample/06-ethernet*).

Please note that there is a different behavior regarding headless operation on the Raspberry Pi 4 compared to earlier models, where the frame buffer initialization succeeds, even if there is no display connected. On the Raspberry Pi 4 there must be a display connected, the initialization of the class CBcmFrameBuffer (and following of the class CScreenDevice) will fail otherwise. Most of the Circle samples are not aware of this and may fail to run without a connected display. You have to modify CKernel::Initialize() for headless operation so that `m_Screen.Initialize()` is not called. `m_Serial` (or maybe class `CNullDevice`) can be used as logging target in this case.

Release 39.2
------------

2019-07-08

This is another intermediate release, which collects the recent changes to the project as a basis for the planned Raspberry Pi 4 support in Circle 40.

News in this release are:

* LittlevGL embedded GUI library (by Gabor Kiss-Vamosi) supported (in addon/littlevgl/).

* The SD card access (EMMC) performance has been remarkable improved.

* USB mass-storage devices (e.g. flash drives) can be removed from the running system now. The application has to call `CDevice::RemoveDevice()` for the device object of the USB mass-storage device, which will be removed afterwards. The file system has to be unmounted before by the application. If FatFs (in addon/fatfs/) is used, the device removal is announced by calling `disk_ioctl (pdrv, CTRL_EJECT, 0)`, where pdrv is the physical drive number (e.g. 1 for the first USB mass-storage device).

* Network initialization can be done in background now to speed-up system initialization. If `CNetSubSystem::Initialize()` is called with the parameter FALSE, it returns quickly without waiting for the Ethernet link to come up and for DHCP to be bound. The network must not be accessed, before `CNetSubSystem::IsRunning()` returns TRUE. This has to be ensured by the application.

Please note that the rudimentary Bluetooth support is deprecated now. There are legal reasons, why it cannot be developed further and because it is currently of very limited use, it will probably be removed soon.

Release 39.1
------------

2019-03-20

This intermediate release comes with a new in-memory-update (chain boot) function and some improvements in detail. Furthermore it is the basis for AArch64 support in the [circle-stdlib](https://github.com/smuehlst/circle-stdlib) project.

The in-memory-update function allows starting a new (Circle-based) kernel image without writing it out to the SD card. This is used to implement a HTTP- and TFTP-based bootloader with Web front-end in *sample/38-bootloader*. Starting larger kernel images with it is much quicker, compared with the serial bootloader. See the *README* file in this directory for details.

The ARM Generic Timer is supported now on Raspberry Pi 2 and 3 as a replacement for the BCM2835 System Timer. This should improve performance and allows using QEMU with AArch64. The system option *USE_PHYSICAL_COUNTER* is enabled by default now.

The relatively rare resource of DMA channels is assigned dynamically now. Lite DMA channels are supported. This allows to use DMA for scrolling the screen much quicker.

There is a new make target "install". If you define `SDCARD = /path` with the full path of your SD card mount point in *Config.mk*, the built kernel image can be copied directly to the SD card. There is a second optional configuration file *Config2.mk* now. Because some Circle-based projects overwrite the file *Config.mk* for configuration, you can set additional non-volatile configuration variables using this new file.

The 39th Step
-------------

2019-02-28

Circle supports the following accelerated graphics APIs now:

* OpenGL ES 1.1 and 2.0
* OpenVG 1.1
* EGL 1.4
* Dispmanx

This has been realized by (partially) porting the Raspberry Pi [userland libraries](https://github.com/raspberrypi/userland), which use the VC4 GPU to render the graphics. Please see the *addon/vc4/interface/* directory and the *README* file in this directory for more details. This support is limited to AArch32 and cannot be built on Raspbian.

The accelerated graphics support requires support for <math.h> functions. To provide this, the *libm.a* standard library is linked now, in case `STDLIB_SUPPORT = 1` is set (default). You need an appropriate toolchain so that it works. See the *Building* section for a link. You may use the <math.h> functions in your own applications too now.

Circle does not support normal USB hot-plugging, but there is a new feature, which allows to detect newly attached USB devices on application request. You can call CDWHCIDevice::ReScanDevices() now, while the application is running, to accomplish this.

The 38th Step
-------------

2019-01-04

The entire Circle project has been ported to the AArch64 architecture. Please see the *AArch64* section below for details! The 32-bit support is still available and will be maintained and developed further.

Moreover a driver for the BMP180 digital pressure sensor has been added to *addon/sensor/*.

Circle creates a short beautified build log now.

The 37th Step
-------------

2018-12-01

In this step the USB gamepad drivers have been totally revised. The PS3, PS4, Xbox 360 Wired, Xbox One and Nintendo Switch Pro gamepads are supported now, including LEDs, rumble and gyroscope. All these drivers support a unique class interface and mapping for the button and axis controls. This should simplify the development of gamepad applications and is used in the new *sample/37-showgamepad*. See the *README* file in this directory for details.

The touchpad of the PS4 gamepad can be used as a mouse device to control normal mouse applications. To implement this, a new unique mouse interface class has been added, which is used by the USB mouse driver too. Therefore existing mouse applications have to be updated as follows:

* Include <circle/input/mouse.h> instead of <circle/usb/usbmouse.h>
* Class of the mouse device is *CMouseDevice* instead of *CUSBMouseDevice*
* Name of the first mouse device is "mouse1" instead of "umouse1"

Finally with this release Circle supports the new Raspberry Pi 3 Model A+.

Release 36.1
------------

2018-11-14

This hotfix release fixes race conditions in the FIQ synchronization, which affected using the FIQ together with EnterCritical() or the class CSpinLock.

The 36th Step
-------------

2018-10-21

In this step the class *CUserTimer* has been added to Circle, which implements a fine grained user programmable interrupt timer. It can generate up to 500000 interrupts per second (if used with FIQ).

*CUserTimer* is used in the new *sample/36-softpwm* to implement Software PWM (Pulse Width Modulation), which can be used to control the brightness of a LED or a servo. See the *README* file in this directory for details.

This release of Circle comes with an updated version of the FatFs module (in *addon/fatfs*). Furthermore there have been fixes to the touchscreen driver and the bootloader tools. Finally there are the new system options *SAVE_VFP_REGS_ON_IRQ* and *SAVE_VFP_REGS_ON_FIQ* in *include/circle/sysconfig.h*, which can be enabled if your application uses floating point registers in interrupt handlers.

You may be interested in the [Alpha GDB server](https://github.com/farjump/Alpha_Raspberry_Pi_Circle) by Farjump, which can be used to source-level debug single-core Circle applications with the GNU debugger (GDB).

Release 35.1
------------

2018-05-15

This intermediate release is done to allow a new release of the [circle-stdlib project](https://github.com/smuehlst/circle-stdlib), which provides C and C++ standard library support for Circle and which has been extended with SSL/TLS support by porting [mbed TLS](https://tls.mbed.org/).

Another new feature in this release is a driver for LCD dot-matrix displays with HD44780 controller in the *addon/display/* subdirectory.

The 35th Step
-------------

2018-04-26

In this step a client for the MQTT IoT protocol has been added to Circle. It is demonstrated in *sample/35-mqttclient*. See the *README* file in this directory for details.

Furthermore a serial bootloader is available now and can be started directly from the Circle build environment. The well-known bootloader by David Welch is used here. See the file *doc/eclipse-support.txt* for instructions on using the bootloader with or without the Eclipse IDE.

There is an incompatible change in behaviour of the CSocket API. Previously when a TCP connection received a FIN (disconnect) and the application called CSocket::Receive(), it did not get an error so far. Because of this, the disconnect has not been detectable. Now an error will be returned in this case. This may cause problems with existing user applications using TCP. You may have to check this.

Release 34.1
------------

2018-03-28

This intermediate release was necessary, because the new Raspberry Pi 3 Model B+ became available. Circle supports it now.

Beside this a driver for the Auxiliary SPI master (SPI1) has been added. The sample/23-spisimple has been updated to support this too. The class CDMAChannel supports the 2D mode now. Furthermore there are some minor improvements in the network subsystem.

The 34th Step
-------------

2018-01-16

In this step a VCHIQ audio service driver and a VCHIQ interface driver have been added to Circle in the *addon/vc4/* subdirectory. They have been ported from Linux and allow to output sound via the HDMI connector. You need a HDMI display which is capable of output sound, to use this.

There is a new Linux kernel driver emulation library in *addon/linux/* which allows to use the VCHIQ interface driver from Linux with only a few changes.

The new *sample/34-sounddevices* can be used to demonstrate the HDMI sound and integrates different sound devices (PWM, I2S, VCHIQ). See the *README* file in this directory for details.

There is a new system option *USE_PHYSICAL_COUNTER* in *include/circle/sysconfig.h* which enables the use of the CPU internal physical counter on the Raspberry Pi 2 and 3, which should improve the system performance, especially if the scheduler is used. It cannot be used with QEMU and that's why it is not enabled by default.

The 33rd Step
-------------

2017-11-18

The main expenditure in this step was spent to prepare Circle for external projects which allow to develop applications which are using C and C++ standard library features. Please see the file *doc/stdlib-support.txt* for details.

Furthermore a syslog client has been added, which allows to send log messages to a syslog server. This is demonstrated in *sample/33-syslog*. See the *README* file in this directory for details.

Circle applications will be linked using the standard library libgcc.a by default now. This library should come with your toolchain. Only if you have problems with that, you can fall back to the previous handling using the setting *STDLIB_SUPPORT=0* in the files *Config.mk* or *Rules.mk*.

If you are using an USB mouse with the mouse cursor in your application, you have to add a call to *CUSBMouseDevice::UpdateCursor()* in your application's main loop so that that cursor can be updated. Please see *sample/10-usbmouse* for an example.

In a few cases it may be important, that the Circle type *boolean* is now equivalent to the C++ type *bool*, which only takes one byte. *boolean* was previously four bytes long.

Release 32.1
------------

2017-10-29

This hotfix release fixes possible SPI DMA errors in the class CSPIMasterDMA.

The 32nd Step
-------------

2017-10-28

In this step a console class has been added to Circle, which allows the easy use of an USB keyboard and a screen together as one device. The console class provides a line editor mode and a raw mode and is demonstrated in *sample/32-i2cshell*, a command line tool for interactive communication with I2C devices. If you are often experimenting with I2C devices, this may be a tool for you. See the *README* file in this directory for details.

Furthermore the Circle USB HCI driver provides an improved compatibility for low-/full-speed USB devices (e.g. keyboards, some did not work properly). Because this update changes the overall system timing, it is not enabled by default to be compatible with existing applications. You should enable the system option *USE_USB_SOF_INTR*, if the improved compatibility is important for your application.

The system options can be found in the file *include/circle/sysconfig.h*. This file has been completely revised and each option is documented now.

Finally there are some improvements in the SPI0 master drivers (polling and DMA) included in this step.

The 31st Step
-------------

2017-08-26

In this step HTTP client support has been added to Circle and is demonstrated in *sample/31-webclient*. See the *README* file in this directory for details.

Furthermore the I2S sound device is supported now with up to 192 KHz sample rate (24-bit I2S audio, tested with PCM5102A DAC). *sample/29-miniorgan* has been updated to be able to use this feature.

The addon/ subdirectory contains a port of the FatFs file system driver (by ChaN), drivers for the HC-SR04 and MPU-6050 sensor devices and special samples to be used with QEMU now.

Finally there are some fixes in the TCP protocol handler included in this step.

Release 30.3
------------

2017-08-01

With the firmware from 2017-07-28 on the ActLED didn't worked on RPi 3 any more and the official touchscreen on RPi 2 and 3. This has been fixed.

Release 30.2
------------

2017-07-30

Multicore support didn't work with recent firmware versions. This has been fixed.

Release 30.1
------------

2017-05-03

This hotfix release fixes an invalid section alignment with -O0. This caused a crash while calling the constructors of static objects in sysinit().

The 30th Step
-------------

2017-04-11

In this step FIQ (fast interrupt) support has been added to Circle. This is used to implement the class CGPIOPinFIQ, which allows fast interrupt-driven event capture from a GPIO pin and is demonstrated in *sample/30-gpiofiq*. See the *README* file in this directory for details.

FIQ support is also used in the class CSerialDevice, which allows an interrupt-driven access to the UART0 device. *sample/29-miniorgan* has been updated to use the UART0 device (at option) as a serial MIDI interface.

Finally in this step QEMU support has been added. See the file *doc/qemu.txt* for details!
