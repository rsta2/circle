#
# Makefile
#
#

#ifeq ($(strip $(DEPTH)), 8)
#LIBCIRCLE = libcircle8
#else
#LIBCIRCLE = libcircle
#endif

CIRCLEHOME = ../..

#OBJS	= main.o kernel.o
	
OBJS = main.o kernel.o Z80SPC.o spcall.o MC6847.o AY8910.o pwmsound.o

LIBS =$(CIRCLEHOME)/lib/usb/libusb.a \
	  $(CIRCLEHOME)/lib/input/libinput.a \
	  $(CIRCLEHOME)/lib/fs/fat/libfatfs.a \
	  $(CIRCLEHOME)/lib/fs/libfs.a \
	  $(CIRCLEHOME)/lib/$(LIBCIRCLE).a 

EXTRACLEAN = sound.h

include ../Rules.mk

kernel.o: sound.h

sound.h: sound.h.gz
	gunzip -c sound.h.gz > sound.h

%.bin: %.asm
	z80asm -fb -l -sa $< 
spcall.c: spc-1000_all.bin spc-1000_ipl.bin
	cat spc-1000_all.bin spc-1000_ipl.bin > spcall.rom
	bin2c.py spcall.rom ROM
