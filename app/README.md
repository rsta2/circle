HD Speccys
==========

A C++ bare metal environment for Raspberry Pi - ZX Spectrum HDMI over SMI

Building
--------

Preparation
- Copy the correct `Config.mk.<pi>` to `Config.mk`
- Run 'npm install' in `tools/flashy`

TODO - automate this part
- Build Circle
  - cd `/`
  - `./makeall`
- Build FatFS
  - cd `addons/fatfs`
  - `make`
- Build SDCard
  - cd `addons/SDCard`
  - `make`
- Build Wifi
  - cd `addons/wlan`
  - `./makeall`
- Download firmware files
  - cd `addons/wlan/firmware`
  - `make`


Standard
- `cd app`
- `./makeall`



Create SD Card
--------------

TODO document better
- Prepare the SD card (should create an SD Card folder, or a script to build it)
- Build the bootloader
- Copy the correct bootloader kernel.img to the SDCard


NOTES
-----

!! Clock speed is limited !!
* The system clock of the ARM CPU on the Raspberry Pi in a bare metal
environment is normally less than the nominal maximum system clock. It is 700
MHz on the Raspberry Pi 1 and Zero and 600 MHz on the Raspberry Pi 2 and 3. If
you need the full speed of your Raspberry Pi, you can use the class CCPUThrottle
to switch the system clock up. This is demonstrated in the sample/26-cpustress.