#
# Download the firmware files to be required for boot (requires wget)
#
# These files must be copied along with the generated kernel.img
#     onto a SD(HC) card with FAT file system.
#

# Use most recent firmware test revision
#FIRMWARE = master

# Use firmware revision built/committed on: Feb 12 2020
FIRMWARE ?= f4b58692fef0b9c16bd4564edb980fff73a758b3

BASEURL = https://github.com/raspberrypi/firmware/blob/$(FIRMWARE)/boot

firmware: clean
	wget -q -O LICENCE.broadcom $(BASEURL)/LICENCE.broadcom?raw=true
	wget -q -O COPYING.linux $(BASEURL)/COPYING.linux?raw=true
	wget -q -O bootcode.bin $(BASEURL)/bootcode.bin?raw=true
	wget -q -O fixup.dat $(BASEURL)/fixup.dat?raw=true
	wget -q -O start.elf $(BASEURL)/start.elf?raw=true
	wget -q -O fixup4.dat $(BASEURL)/fixup4.dat?raw=true
	wget -q -O start4.elf $(BASEURL)/start4.elf?raw=true
	wget -q -O bcm2711-rpi-4-b.dtb $(BASEURL)/bcm2711-rpi-4-b.dtb?raw=true

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

armstub64:
	make -C armstub
	cp armstub/armstub8-rpi4.bin .

clean:
	rm -f bootcode.bin fixup*.dat start*.elf bcm2711-rpi-4-b.dtb kernel*.img armstub*.bin LICENCE.broadcom COPYING.linux
	make clean -C "../tools/bootloader"
	make clean -C armstub
