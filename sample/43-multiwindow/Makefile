#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= main.o kernel.o multicoredemo.o graphicshape.o mandelbrot.o

LIBS	= $(CIRCLEHOME)/addon/lvgl/liblvgl.a \
	  $(CIRCLEHOME)/lib/usb/libusb.a \
	  $(CIRCLEHOME)/lib/input/libinput.a \
	  $(CIRCLEHOME)/lib/fs/libfs.a \
	  $(CIRCLEHOME)/lib/libcircle.a

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
