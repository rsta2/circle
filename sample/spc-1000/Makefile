#
# Makefile
#

CIRCLEHOME = ../..

	
OBJS = main.o kernel.o Z80SPC.o spcall.o MC6847.o

LIBS =$(CIRCLEHOME)/lib/usb/libusb.a \
	  $(CIRCLEHOME)/lib/input/libinput.a \
	  $(CIRCLEHOME)/lib/fs/fat/libfatfs.a \
	  $(CIRCLEHOME)/lib/fs/libfs.a \
	  $(CIRCLEHOME)/lib/libcircle.a

include ../Rules.mk

%.bin: %.asm
	z80asm -fb -l -sa $< 
spcall.c: spc-1000_all.bin spc-1000_ipl.bin
	cat spc-1000_all.bin spc-1000_ipl.bin > spcall.rom
	bin2c.py spcall.rom ROM
