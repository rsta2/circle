#
# Makefile
#

#USE_ST7789 = 1

CIRCLEHOME = ../..

OBJS	= main.o kernel.o

LIBS	= $(CIRCLEHOME)/lib/usb/libusb.a \
	  $(CIRCLEHOME)/lib/input/libinput.a \
	  $(CIRCLEHOME)/lib/fs/libfs.a \
	  $(CIRCLEHOME)/lib/libcircle.a

ifeq ($(strip $(USE_ST7789)),1)
LIBS	+= $(CIRCLEHOME)/addon/display/libdisplay.a

CFLAGS	+= -DUSE_ST7789=$(USE_ST7789)
endif

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
