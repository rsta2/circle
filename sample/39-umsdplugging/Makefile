#
# Makefile
#

#USE_FATFS = 1

CIRCLEHOME = ../..

OBJS	= main.o kernel.o

ifeq ($(strip $(USE_FATFS)),1)
LIBS	= $(CIRCLEHOME)/addon/fatfs/libfatfs.a

CFLAGS += -DUSE_FATFS=$(USE_FATFS)
else
LIBS	= $(CIRCLEHOME)/lib/fs/fat/libfatfs.a
endif

LIBS	+= $(CIRCLEHOME)/lib/usb/libusb.a \
	   $(CIRCLEHOME)/lib/input/libinput.a \
	   $(CIRCLEHOME)/lib/fs/libfs.a \
	   $(CIRCLEHOME)/lib/libcircle.a

include $(CIRCLEHOME)/sample/Rules.mk

-include $(DEPS)
