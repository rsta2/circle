#
# Makefile
#

CIRCLEHOME = ../../..

CPPFLAGS += -Wno-write-strings

OBJS	= main.o kernel.o controlwindow.o scopewindow.o scopeconfig.o timeline.o recorder.o sampler.o

LIBS	= ../libugui.a \
	  $(CIRCLEHOME)/lib/usb/libusb.a \
	  $(CIRCLEHOME)/lib/input/libinput.a \
	  $(CIRCLEHOME)/lib/fs/libfs.a \
	  $(CIRCLEHOME)/lib/libcircle.a

include $(CIRCLEHOME)/sample/Rules.mk
