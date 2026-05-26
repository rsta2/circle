#
# Makefile
#

#DSI_DISPLAY = 0		# DISP# port to use

CIRCLEHOME = ../..

OBJS	= main.o kernel.o

LIBS	= $(CIRCLEHOME)/lib/usb/libusb.a \
	  $(CIRCLEHOME)/lib/input/libinput.a \
	  $(CIRCLEHOME)/lib/fs/libfs.a \
	  $(CIRCLEHOME)/lib/libcircle.a

ifneq ($(strip $(DSI_DISPLAY)),)
LIBS	+= $(CIRCLEHOME)/addon/rp1dsi/librp1dsi.a

CFLAGS	+= -DDSI_DISPLAY=$(DSI_DISPLAY)
endif

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
