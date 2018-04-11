-include ../../Config.mk

PREFIX ?= arm-none-eabi-
FLASHBAUD ?= 115200

ARMGNU ?= $(PREFIX)

COPS = -Wall -O2 -nostdlib -nostartfiles -ffreestanding -DDEFAULTBAUD=$(FLASHBAUD)

all: kernel.img kernel7.img

clean :
	rm -f *.o
	rm -f *.elf
	rm -f *.lst
	rm -f *.img

vectors.o : vectors.s
	$(ARMGNU)as vectors.s -o vectors.o

bootloader07.o : bootloader07.c
	$(ARMGNU)gcc $(COPS) -c bootloader07.c -o bootloader07.o

periph.o : periph.c BCM2835.h
	$(ARMGNU)gcc $(COPS) -c periph.c -o periph.o

periph7.o : periph.c BCM2836.h
	$(ARMGNU)gcc $(COPS) -c periph.c -o periph7.o -DRPI2

kernel.img : loader vectors.o periph.o bootloader07.o 
	$(ARMGNU)ld vectors.o periph.o bootloader07.o -T loader -o bootloader07_rpi1.elf
	$(ARMGNU)objdump -D bootloader07_rpi1.elf > bootloader07_rpi1.lst
	$(ARMGNU)objcopy bootloader07_rpi1.elf -O binary kernel.img

kernel7.img : loader vectors.o periph7.o bootloader07.o 
	$(ARMGNU)ld vectors.o periph7.o bootloader07.o -T loader -o bootloader07_rpi2.elf
	$(ARMGNU)objdump -D bootloader07_rpi2.elf > bootloader07_rpi2.lst
	$(ARMGNU)objcopy bootloader07_rpi2.elf -O binary kernel7.img
