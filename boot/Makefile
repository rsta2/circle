#
# Download the firmware files to be required for boot (requires wget)
#
# These files must be copied along with the generated kernel.img
#     onto a SD(HC) card with FAT file system.
#

# Use most recent firmware test revision
#FIRMWARE = master

# Use firmware revision built/committed on: Feb 25 2021
FIRMWARE ?= 5985247fb75681985547641d66196c77499f26b9

BASEURL = https://github.com/raspberrypi/firmware/blob/$(FIRMWARE)/boot

firmware: clean
	wget -q -O LICENCE.broadcom $(BASEURL)/LICENCE.broadcom?raw=true
	wget -q -O COPYING.linux $(BASEURL)/COPYING.linux?raw=true
	wget -q -O bootcode.bin $(BASEURL)/bootcode.bin?raw=true
	wget -q -O fixup.dat $(BASEURL)/fixup.dat?raw=true
	wget -q -O fixup_cd.dat $(BASEURL)/fixup_cd.dat?raw=true
	wget -q -O start.elf $(BASEURL)/start.elf?raw=true
	wget -q -O start_cd.elf $(BASEURL)/start_cd.elf?raw=true
	wget -q -O fixup4.dat $(BASEURL)/fixup4.dat?raw=true
	wget -q -O fixup4cd.dat $(BASEURL)/fixup4cd.dat?raw=true
	wget -q -O start4.elf $(BASEURL)/start4.elf?raw=true
	wget -q -O start4cd.elf $(BASEURL)/start4cd.elf?raw=true
	wget -q -O bcm2711-rpi-400.dtb $(BASEURL)/bcm2711-rpi-400.dtb?raw=true
	wget -q -O bcm2711-rpi-4-b.dtb $(BASEURL)/bcm2711-rpi-4-b.dtb?raw=true
	wget -q -O bcm2711-rpi-cm4.dtb $(BASEURL)/bcm2711-rpi-cm4.dtb?raw=true

all: firmware bootloader

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

clean:
	rm -f bootcode.bin fixup*.dat start*.elf bcm*.dtb kernel*.img armstub*.bin LICENCE.broadcom COPYING.linux
	make clean -C "../tools/bootloader"
	make clean -C armstub
