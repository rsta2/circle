-include ../../Config.mk
-include ../../Config2.mk

PREFIX ?= arm-none-eabi-
PREFIX64 ?= aarch64-none-elf-
FLASHBAUD ?= 115200

ARMGNU ?= $(PREFIX)
ARMGNU64 ?= $(PREFIX64)

COPS = -Wall -O2 -nostdlib -nostartfiles -ffreestanding -DDEFAULTBAUD=$(FLASHBAUD)
COPS32 = -mno-unaligned-access
COPS64 = -mstrict-align

all: kernel.img kernel7.img kernel7l.img

clean :
	rm -f *.o
	rm -f *.elf
	rm -f *.lst
	rm -f *.img

vectors.o : vectors.s
	$(ARMGNU)as vectors.s -o vectors.o

vectors64.o : vectors64.s
	$(ARMGNU64)as vectors64.s -o vectors64.o

bootloader07.o : bootloader07.c
	$(ARMGNU)gcc $(COPS) $(COPS32) -c bootloader07.c -o bootloader07.o -DAARCH=32

bootloader07_64.o : bootloader07.c
	$(ARMGNU64)gcc $(COPS) $(COPS64) -c bootloader07.c -o bootloader07_64.o -DAARCH=64

periph.o : periph.c BCM2835.h
	$(ARMGNU)gcc $(COPS) $(COPS32) -c periph.c -o periph.o -DAARCH=32

periph7.o : periph.c BCM2836.h
	$(ARMGNU)gcc $(COPS) $(COPS32) -c periph.c -o periph7.o -DRPI2 -DAARCH=32

periph7_rpi4.o : periph.c BCM2711.h
	$(ARMGNU)gcc $(COPS) $(COPS32) -c periph.c -o periph7_rpi4.o -DRPI4 -DAARCH=32

periph64.o : periph.c BCM2836.h
	$(ARMGNU64)gcc $(COPS) $(COPS64) -c periph.c -o periph64.o -DAARCH=64

periph64_rpi4.o : periph.c BCM2711.h
	$(ARMGNU64)gcc $(COPS) $(COPS64) -c periph.c -o periph64_rpi4.o -DRPI4 -DAARCH=64

kernel.img : loader vectors.o periph.o bootloader07.o
	$(ARMGNU)ld vectors.o periph.o bootloader07.o -T loader -o bootloader07_rpi1.elf
	$(ARMGNU)objdump -D bootloader07_rpi1.elf > bootloader07_rpi1.lst
	$(ARMGNU)objcopy bootloader07_rpi1.elf -O binary kernel.img

kernel7.img : loader vectors.o periph7.o bootloader07.o 
	$(ARMGNU)ld vectors.o periph7.o bootloader07.o -T loader -o bootloader07_rpi2.elf
	$(ARMGNU)objdump -D bootloader07_rpi2.elf > bootloader07_rpi2.lst
	$(ARMGNU)objcopy bootloader07_rpi2.elf -O binary kernel7.img

kernel7l.img : loader vectors.o periph7_rpi4.o bootloader07.o
	$(ARMGNU)ld vectors.o periph7_rpi4.o bootloader07.o -T loader -o bootloader07_rpi4.elf
	$(ARMGNU)objdump -D bootloader07_rpi4.elf > bootloader07_rpi4.lst
	$(ARMGNU)objcopy bootloader07_rpi4.elf -O binary kernel7l.img

kernel8.img : loader64 vectors64.o periph64.o bootloader07_64.o
	$(ARMGNU64)ld vectors64.o periph64.o bootloader07_64.o -T loader64 -o bootloader07_64.elf
	$(ARMGNU64)objdump -D bootloader07_64.elf > bootloader07_64.lst
	$(ARMGNU64)objcopy bootloader07_64.elf -O binary kernel8.img

kernel8-rpi4.img : loader64 vectors64.o periph64_rpi4.o bootloader07_64.o
	$(ARMGNU64)ld vectors64.o periph64_rpi4.o bootloader07_64.o -T loader64 -o bootloader07_64_rpi4.elf
	$(ARMGNU64)objdump -D bootloader07_64_rpi4.elf > bootloader07_64_rpi4.lst
	$(ARMGNU64)objcopy bootloader07_64_rpi4.elf -O binary kernel8-rpi4.img
