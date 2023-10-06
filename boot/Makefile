#
# Download the firmware files to be required for boot (requires wget)
#
# These files must be copied along with the generated kernel.img
#     onto a SD(HC) card with FAT file system.
#

-include ../Config.mk
-include ../Config2.mk

# Use most recent firmware test revision
#FIRMWARE = master

# Use firmware revision built/committed on: Mar 17 2023
FIRMWARE ?= a585b376a2e7e657287543d196ae1f8881ede559

BASEURL = https://github.com/raspberrypi/firmware/blob/$(FIRMWARE)/boot

FILES = LICENCE.broadcom COPYING.linux \
	bootcode.bin start.elf fixup.dat start4.elf fixup4.dat \
	start_cd.elf fixup_cd.dat start4cd.elf fixup4cd.dat \
	bcm2710-rpi-zero-2-w.dtb bcm2711-rpi-4-b.dtb bcm2711-rpi-400.dtb bcm2711-rpi-cm4.dtb

FILES32 = armstub7-rpi4.bin kernel.img kernel7.img kernel7l.img
FILES64 = armstub8-rpi4.bin kernel8.img kernel8-rpi4.img

firmware: clean
	@for file in $(FILES) ; \
	do \
		echo Downloading $$file ... ; \
		wget -q -O $$file $(BASEURL)/$$file?raw=true ; \
	done

all: firmware armstub bootloader

all64: firmware armstub64 bootloader64

bootloader:
	make -C "../tools/bootloader"
	cp ../tools/bootloader/kernel.img .
	cp ../tools/bootloader/kernel7.img .
	cp ../tools/bootloader/kernel7l.img .

bootloader64:
	make -C "../tools/bootloader" kernel8.img
	make -C "../tools/bootloader" kernel8-rpi4.img
	cp ../tools/bootloader/kernel8.img .
	cp ../tools/bootloader/kernel8-rpi4.img .

.PHONY: armstub
armstub:
	make -C armstub armstub7-rpi4.bin
	cp armstub/armstub7-rpi4.bin .

armstub64:
	make -C armstub armstub8-rpi4.bin
	cp armstub/armstub8-rpi4.bin .

ifneq ($(strip $(SDCARD)),)
install: firmware armstub bootloader armstub64 bootloader64
	@for file in $(FILES) $(FILES32) $(FILES64) ; \
	do \
		echo Copying $$file ... ; \
		cp $$file $(SDCARD) ; \
	done
	cp config32.txt $(SDCARD)
	cp config64.txt $(SDCARD)
	sync
	@echo "You must rename config32.txt or config64.txt to config.txt on the SD card!"

install32: firmware armstub bootloader
	@for file in $(FILES) $(FILES32) ; \
	do \
		echo Copying $$file ... ; \
		cp $$file $(SDCARD) ; \
	done
	cp config32.txt $(SDCARD)/config.txt
	sync

install64: firmware armstub64 bootloader64
	@for file in $(FILES) $(FILES64) ; \
	do \
		echo Copying $$file ... ; \
		cp $$file $(SDCARD) ; \
	done
	cp config64.txt $(SDCARD)/config.txt
	sync
endif

clean:
	rm -f bootcode.bin fixup*.dat start*.elf bcm*.dtb kernel*.img armstub*.bin LICENCE.broadcom COPYING.linux
	make clean -C "../tools/bootloader"
	make clean -C armstub
