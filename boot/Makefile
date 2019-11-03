#
# Download the firmware files to be required for boot (requires wget)
#
# These files must be copied along with the generated kernel.img
#     onto a SD(HC) card with FAT file system.
#

firmware: clean
	wget -q -O LICENCE.broadcom https://github.com/raspberrypi/firmware/blob/master/boot/LICENCE.broadcom?raw=true
	wget -q -O COPYING.linux https://github.com/raspberrypi/firmware/blob/master/boot/COPYING.linux?raw=true
	wget -q -O bootcode.bin https://github.com/raspberrypi/firmware/blob/master/boot/bootcode.bin?raw=true
	wget -q -O fixup.dat https://github.com/raspberrypi/firmware/blob/master/boot/fixup.dat?raw=true
	wget -q -O start.elf https://github.com/raspberrypi/firmware/blob/master/boot/start.elf?raw=true
	wget -q -O fixup4.dat https://github.com/raspberrypi/firmware/blob/master/boot/fixup4.dat?raw=true
	wget -q -O start4.elf https://github.com/raspberrypi/firmware/blob/master/boot/start4.elf?raw=true
	wget -q -O bcm2711-rpi-4-b.dtb https://github.com/raspberrypi/firmware/blob/master/boot/bcm2711-rpi-4-b.dtb?raw=true

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

clean:
	rm -f bootcode.bin fixup*.dat start*.elf bcm2711-rpi-4-b.dtb kernel*.img LICENCE.broadcom COPYING.linux
	make clean -C "../tools/bootloader"
