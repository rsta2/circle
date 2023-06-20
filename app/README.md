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