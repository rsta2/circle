#
# Makefile
#

CIRCLEHOME = ../../../..

OBJS	= main.o kernel.o

LIBS	= $(CIRCLEHOME)/addon/display/libdisplay.a \
	  $(CIRCLEHOME)/lib/usb/libusb.a \
	  $(CIRCLEHOME)/lib/input/libinput.a \
	  $(CIRCLEHOME)/lib/fs/libfs.a \
	  $(CIRCLEHOME)/lib/libcircle.a

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
