#
# Makefile
#

CIRCLEHOME = ../..

CFLAGS = -I Z80 -I EMULib
	
OBJS = fMSX.o I8251.o Menu.o MSX.o Patch.o V9938.o

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
