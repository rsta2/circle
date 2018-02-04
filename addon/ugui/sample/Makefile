#
# Makefile
#

CIRCLEHOME = ../../..

CPPFLAGS += -Wno-write-strings

OBJS	= main.o kernel.o controlwindow.o scopewindow.o scopeconfig.o timeline.o recorder.o sampler.o

LIBS	= ../libugui$(RASPPI).a \
	  $(CIRCLEHOME)/lib/usb/libusb$(RASPPI).a \
	  $(CIRCLEHOME)/lib/input/libinput$(RASPPI).a \
	  $(CIRCLEHOME)/lib/fs/libfs$(RASPPI).a \
	  $(CIRCLEHOME)/lib/libcircle$(RASPPI).a

include $(CIRCLEHOME)/sample/Rules.mk
